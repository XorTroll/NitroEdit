#include <ntr/fmt/nfs/nfs_NitroFs.hpp>
#include <ntr/fs/fs_Stdio.hpp>

namespace ntr::fmt::nfs {

    namespace {

        Result ReadNitroDirectoryImpl(const size_t fat_data_offset, const size_t fnt_data_offset, fs::BinaryFile &bf, NitroDirectory &nitro_dir, const u16 dir_id) {
            const auto dir_idx = dir_id & 0xfff;
            NTR_R_TRY(bf.SetAbsoluteOffset(fnt_data_offset + dir_idx * sizeof(DirectoryNameTableEntry)));

            DirectoryNameTableEntry dir_entry;
            NTR_R_TRY(bf.Read(dir_entry));

            NTR_R_TRY(bf.SetAbsoluteOffset(fnt_data_offset + dir_entry.start));

            auto cur_file_id = dir_entry.id;
            while(true) {
                size_t entry_offset;
                NTR_R_TRY(bf.GetAbsoluteOffset(entry_offset));
                u8 entry_val;
                NTR_R_TRY(bf.Read(entry_val));

                if(entry_val == 0) {
                    // End of directory
                    break;
                }
                else if(entry_val < 128) {
                    // File
                    NitroFile nitro_file = {};
                    nitro_file.entry_offset = entry_offset;
                    const auto name_len = entry_val;
                    NTR_R_TRY(bf.MoveOffset(name_len));

                    size_t old_offset;
                    NTR_R_TRY(bf.GetAbsoluteOffset(old_offset));
                    NTR_R_TRY(bf.SetAbsoluteOffset(fat_data_offset + cur_file_id * sizeof(FileAllocationTableEntry)));
                    FileAllocationTableEntry fat_entry;
                    NTR_R_TRY(bf.Read(fat_entry));
                    NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));

                    nitro_file.offset = fat_entry.file_start;
                    nitro_file.size = fat_entry.file_end - fat_entry.file_start;
                    nitro_dir.files.push_back(nitro_file);
                    cur_file_id++;
                }
                else {
                    // Directory
                    NitroDirectory nitro_subdir = {};
                    nitro_subdir.is_root = false;
                    nitro_subdir.entry_offset = entry_offset;
                    const auto name_len = entry_val - 128;
                    NTR_R_TRY(bf.MoveOffset(name_len));

                    u16 sub_dir_id;
                    NTR_R_TRY(bf.Read(sub_dir_id));

                    size_t old_offset;
                    NTR_R_TRY(bf.GetAbsoluteOffset(old_offset));
                    NTR_R_TRY(ReadNitroDirectoryImpl(fat_data_offset, fnt_data_offset, bf, nitro_subdir, sub_dir_id));
                    NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));

                    nitro_dir.dirs.push_back(nitro_subdir);
                }
            }

            NTR_R_SUCCEED();
        }

        void UpdateNitroDirectoryOffsetsImpl(nfs::NitroDirectory &dir, const NitroFile &this_file, const size_t new_file_size, const size_t pad_size) {
            const ssize_t size_diff = new_file_size + pad_size - this_file.size;
            for(auto &file : dir.files) {
                if(file.offset > this_file.offset) {
                    file.offset += size_diff;
                }
                else if(file.offset == this_file.offset) {
                    file.size = new_file_size;
                }
            }
            for(auto &subdir : dir.dirs) {
                UpdateNitroDirectoryOffsetsImpl(subdir, this_file, new_file_size, pad_size);
            }
        }

    }

    Result NitroEntryBase::GetName(fs::BinaryFile &base_bf, std::string &out_name) const {
        if(this->entry_offset == UINT32_MAX) {
            out_name = RootDirectoryPseudoName;
            NTR_R_SUCCEED();
        }
        else {
            NTR_R_TRY(base_bf.SetAbsoluteOffset(this->entry_offset));

            u8 entry_val;
            NTR_R_TRY(base_bf.Read(entry_val));
            char name[128] = {};
            if(entry_val > 128) {
                entry_val -= 128;
            }
            NTR_R_TRY(base_bf.ReadDataExact(name, entry_val));
            out_name.assign(name);
            NTR_R_SUCCEED();
        }        
    }

    Result NitroFile::Read(fs::BinaryFile &base_bf, const size_t offset, void *read_buf, const size_t read_size, size_t &out_read_size) const {
        size_t old_offset;
        NTR_R_TRY(base_bf.GetAbsoluteOffset(old_offset));
        NTR_R_TRY(base_bf.SetAbsoluteOffset(this->offset + offset));
        ScopeGuard on_exit_cleanup([&]() {
            base_bf.SetAbsoluteOffset(old_offset);
        });

        const auto r_size = std::min(this->size, read_size);
        NTR_R_TRY(base_bf.ReadData(read_buf, r_size, out_read_size));
        
        NTR_R_SUCCEED();
    }

    Result ReadNitroFsFrom(const size_t fat_data_offset, const size_t fnt_data_offset, fs::BinaryFile &bf, NitroDirectory &out_fs_root_dir) {
        out_fs_root_dir.is_root = true;
        out_fs_root_dir.entry_offset = UINT32_MAX;
        return ReadNitroDirectoryImpl(fat_data_offset, fnt_data_offset, bf, out_fs_root_dir, InitialDirectoryId);
    }

    Result NitroFsFileFormat::LookupFile(const std::string &path, NitroFile &out_file) const {
        return this->DoWithReadFile([&](fs::BinaryFile &bf) -> Result {
            const auto *cur_dir = std::addressof(this->nitro_fs);
            auto pos_init = 0;
            auto pos_found = 0;
            auto found = true;
            std::string token = "";
            while(pos_found >= 0) {
                pos_found = path.find('/', pos_init);
                token = path.substr(pos_init, pos_found - pos_init);

                pos_init = pos_found + 1;

                if(!found) {
                    // Last directory was not found, and there is another token left
                    NTR_R_FAIL(ResultNitroFsDirectoryNotFound);
                }

                found = false;
                for(const auto &dir : cur_dir->dirs) {
                    std::string dir_name;
                    NTR_R_TRY(dir.GetName(bf, dir_name));
                    if(token == dir_name) {
                        cur_dir = std::addressof(dir);
                        found = true;
                        break;
                    }
                }
            }

            // If everything went alright and the path is valid, last token must be a file at cur_dir
            for(const auto &file : cur_dir->files) {
                std::string file_name;
                NTR_R_TRY(file.GetName(bf, file_name));
                if(token == file_name) {
                    out_file = file;
                    NTR_R_SUCCEED();
                }
            }
            NTR_R_FAIL(ResultNitroFsFileNotFound);
        });
    }

    Result NitroFsFileFormat::GetName(const NitroEntryBase &entry, std::string &out_name) const {
        fs::BinaryFile bf;
        NTR_R_TRY(bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp));

        NTR_R_TRY(entry.GetName(bf, out_name));
        NTR_R_SUCCEED();
    }

    Result NitroFsFileFormat::SaveFileSystem() {
        std::vector<std::string> ext_fs_files;
        NTR_R_TRY(fs::ListAllStdioFiles(this->ext_fs_root_path, ext_fs_files));

        auto w_path = this->write_path;
        auto w_file_handle = this->write_file_handle;

        const auto write_on_self = (w_file_handle == nullptr) || w_path.empty();

        if(write_on_self) {
            w_file_handle = std::make_shared<fs::StdioFileHandle>();
            w_path = this->ext_fs_root_path + "_tmp_" + fs::GetFileName(this->read_path);
        }

        if(!ext_fs_files.empty()) {
            std::vector<std::pair<std::string, NitroFile>> nfs_files;
            for(const auto &ext_fs_file : ext_fs_files) {
                const auto nfs_path = this->GetBasePath(ext_fs_file);
                NitroFile nfs_file = {};
                NTR_R_TRY(this->LookupFile(nfs_path, nfs_file));
                nfs_files.push_back(std::make_pair(ext_fs_file, nfs_file));
            }

            std::sort(nfs_files.begin(), nfs_files.end(), [](const std::pair<std::string, NitroFile> &p_a, const std::pair<std::string, NitroFile> &p_b) -> bool {
                return p_a.second.offset < p_b.second.offset;
            });

            fs::BinaryFile w_bf;
            NTR_R_TRY(w_bf.Open(w_file_handle, w_path, fs::OpenMode::Write, this->comp));

            fs::BinaryFile r_bf;
            NTR_R_TRY(r_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp));

            size_t orig_self_size;
            NTR_R_TRY(r_bf.GetSize(orig_self_size));
            const auto base_offset = this->GetBaseOffset();
            const auto pre_file_data_offset = 0;
            const auto pre_file_data_size = base_offset + nfs_files.front().second.offset;
            const auto post_file_data_offset = base_offset + nfs_files.back().second.offset + nfs_files.back().second.size;
            NTR_R_TRY(r_bf.SetAbsoluteOffset(pre_file_data_offset));
            NTR_R_TRY(w_bf.CopyFrom(r_bf, pre_file_data_size));

            const auto fat_entries_offset = this->GetFatEntriesOffset();
            const auto fat_entry_count = this->GetFatEntryCount();
            const auto nfs_file_count = nfs_files.size();
            auto fat_entries = util::NewArray<FileAllocationTableEntry>(fat_entry_count);
            NTR_R_TRY(r_bf.SetAbsoluteOffset(fat_entries_offset));
            NTR_R_TRY(r_bf.ReadDataExact(fat_entries, sizeof(FileAllocationTableEntry) * fat_entry_count));
            ssize_t size_diff = 0;

            u32 i = 0;
            for(auto &[ext_fs_file, nfs_file] : nfs_files) {
                const auto nfs_file_w_offset = nfs_file.offset + size_diff;
                const auto nfs_file_r_offset = nfs_file.offset;
                size_t new_file_size;
                NTR_R_TRY(fs::GetStdioFileSize(ext_fs_file, new_file_size));
                {
                    fs::BinaryFile d_bf;
                    NTR_R_TRY(d_bf.Open(std::make_shared<fs::StdioFileHandle>(), ext_fs_file, fs::OpenMode::Read));
                    NTR_R_TRY(w_bf.SetAbsoluteOffset(base_offset + nfs_file_w_offset));
                    NTR_R_TRY(w_bf.CopyFrom(d_bf, new_file_size));
                }

                size_t data_align;
                size_t pad_size = 0;
                if(this->GetAlignmentBetweenFileData(data_align)) {
                    NTR_R_TRY(w_bf.WriteEnsureAlignment(data_align, pad_size));
                }

                const auto has_next_file = (i + 1) < nfs_file_count;
                if(has_next_file) {
                    const auto &[next_ext_fs_file, next_nfs_file] = nfs_files[i + 1];
                    const auto data_between_files_offset = base_offset + nfs_file_r_offset + nfs_file.size;
                    auto data_between_files_size = next_nfs_file.offset - nfs_file_r_offset - nfs_file.size;
                    if(this->GetAlignmentBetweenFileData(data_align)) {
                        size_t cur_w_bf_offset;
                        NTR_R_TRY(w_bf.GetAbsoluteOffset(cur_w_bf_offset));
                        auto tmp_align_offset = cur_w_bf_offset + data_between_files_size;
                        auto data_between_files_pad_size = 0;
                        while(!util::IsAlignedTo(tmp_align_offset, data_align)) {
                            tmp_align_offset++;
                            data_between_files_pad_size++;
                        }
                        data_between_files_size += data_between_files_pad_size;
                    }
                    
                    NTR_R_TRY(r_bf.SetAbsoluteOffset(data_between_files_offset));
                    NTR_R_TRY(w_bf.CopyFrom(r_bf, data_between_files_size));
                }

                const ssize_t cur_size_diff = new_file_size + pad_size - nfs_file.size;

                // const auto old_offset = w_bf.GetAbsoluteOffset();
                for(size_t i = 0; i < fat_entry_count; i++) {
                    auto &cur_entry = fat_entries[i];
                    if(cur_entry.file_start > nfs_file_w_offset) {
                        // It's a file after our current file, advance cur_size_diff the file position
                        cur_entry.file_start += cur_size_diff;
                        cur_entry.file_end += cur_size_diff;
                    }
                    else if(cur_entry.file_start == nfs_file_w_offset) {
                        // It's our current file, set the new size
                        cur_entry.file_end = cur_entry.file_start + new_file_size;
                    }
                }

                nfs_file.size = new_file_size;

                /* update nitrofs's file offsets too */
                // TODO: fix this
                // UpdateNitroDirectoryOffsetsImpl(this->nitro_fs, nfs_file, new_file_size, pad_size);

                size_diff += cur_size_diff;
                i++;
            }

            size_t old_offset;
            NTR_R_TRY(w_bf.GetAbsoluteOffset(old_offset));
            NTR_R_TRY(w_bf.SetAbsoluteOffset(fat_entries_offset));
            NTR_R_TRY(w_bf.WriteData(fat_entries, sizeof(FileAllocationTableEntry) * fat_entry_count));
            delete[] fat_entries;
            NTR_R_TRY(w_bf.SetAbsoluteOffset(old_offset));

            const auto post_file_data_size = orig_self_size - post_file_data_offset;

            NTR_R_TRY(r_bf.SetAbsoluteOffset(post_file_data_offset));
            NTR_R_TRY(w_bf.CopyFrom(r_bf, post_file_data_size));

            /* format-specific final writes */
            NTR_R_TRY(this->OnFileSystemWrite(w_bf, size_diff));
        }
        else {
            fs::BinaryFile w_bf;
            NTR_R_TRY(w_bf.Open(w_file_handle, w_path, fs::OpenMode::Write, this->comp));

            fs::BinaryFile r_bf;
            NTR_R_TRY(r_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp));

            size_t r_file_size;
            NTR_R_TRY(r_bf.GetSize(r_file_size));
            NTR_R_TRY(w_bf.CopyFrom(r_bf, r_file_size));

            NTR_R_TRY(this->OnFileSystemWrite(w_bf, 0));
        }

        if(write_on_self) {
            fs::BinaryFile w_bf;
            NTR_R_TRY(w_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Write));

            fs::BinaryFile r_bf;
            NTR_R_TRY(r_bf.Open(w_file_handle, w_path, fs::OpenMode::Read));

            size_t r_file_size;
            NTR_R_TRY(r_bf.GetSize(r_file_size));
            NTR_R_TRY(w_bf.CopyFrom(r_bf, r_file_size));

            NTR_R_TRY(fs::DeleteStdioFile(w_path));

            w_file_handle = nullptr;
            w_path.clear();
        }

        for(const auto &ext_fs_file : ext_fs_files) {
            fs::DeleteStdioFile(ext_fs_file);
        }

        NTR_R_SUCCEED();
    }

}