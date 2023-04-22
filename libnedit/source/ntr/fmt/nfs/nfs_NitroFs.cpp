#include <ntr/fmt/nfs/nfs_NitroFs.hpp>
#include <ntr/fs/fs_Stdio.hpp>

namespace ntr::fmt::nfs {

    namespace {

        bool ReadNitroDirectoryImpl(const size_t fat_data_offset, const size_t fnt_data_offset, fs::BinaryFile &bf, NitroDirectory &nitro_dir, const u16 dir_id) {
            const auto dir_idx = dir_id & 0xfff;
            if(!bf.SetAbsoluteOffset(fnt_data_offset + dir_idx * sizeof(DirectoryNameTableEntry))) {
                return false;
            }

            DirectoryNameTableEntry dir_entry;
            if(!bf.Read(dir_entry)) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(fnt_data_offset + dir_entry.start)) {
                return false;
            }

            auto cur_file_id = dir_entry.id;
            while(true) {
                const auto entry_offset = bf.GetAbsoluteOffset();
                u8 entry_val;
                if(!bf.Read(entry_val)) {
                    return false;
                }

                if(entry_val == 0) {
                    // End of directory
                    break;
                }
                else if(entry_val < 128) {
                    // File
                    NitroFile nitro_file = {};
                    nitro_file.entry_offset = entry_offset;
                    const auto name_len = entry_val;
                    if(!bf.MoveOffset(name_len)) {
                        return false;
                    }

                    const auto old_offset = bf.GetAbsoluteOffset();
                    if(!bf.SetAbsoluteOffset(fat_data_offset + cur_file_id * sizeof(FileAllocationTableEntry))) {
                        return false;
                    }
                    FileAllocationTableEntry fat_entry;
                    if(!bf.Read(fat_entry)) {
                        return false;
                    }
                    if(!bf.SetAbsoluteOffset(old_offset)) {
                        return false;
                    }

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
                    if(!bf.MoveOffset(name_len)) {
                        return false;
                    }

                    u16 sub_dir_id;
                    if(!bf.Read(sub_dir_id)) {
                        return false;
                    }

                    const auto old_offset = bf.GetAbsoluteOffset();
                    if(!ReadNitroDirectoryImpl(fat_data_offset, fnt_data_offset, bf, nitro_subdir, sub_dir_id)) {
                        return false;
                    }
                    if(!bf.SetAbsoluteOffset(old_offset)) {
                        return false;
                    }

                    nitro_dir.dirs.push_back(nitro_subdir);
                }
            }

            return true;
        }

        void UpdateNitroDirectoryOffsetsImpl(nfs::NitroDirectory &dir, const NitroFile &this_file, const size_t new_file_size, const size_t pad_count) {
            const ssize_t size_diff = new_file_size + pad_count - this_file.size;
            for(auto &file : dir.files) {
                if(file.offset > this_file.offset) {
                    file.offset += size_diff;
                }
                else if(file.offset == this_file.offset) {
                    file.size = new_file_size;
                }
            }
            for(auto &subdir : dir.dirs) {
                UpdateNitroDirectoryOffsetsImpl(subdir, this_file, new_file_size, pad_count);
            }
        }

    }

    std::string NitroEntryBase::GetName(fs::BinaryFile &base_bf) const {
        if(this->entry_offset == UINT32_MAX) {
            return RootDirectoryPseudoName;
        }
        if(base_bf.SetAbsoluteOffset(this->entry_offset)) {
            u8 entry_val;
            if(base_bf.Read(entry_val)) {
                char name[128] = {};
                if(entry_val > 128) {
                    entry_val -= 128;
                }
                if(base_bf.ReadData(name, entry_val)) {
                    return std::string(name, entry_val);
                }
            }
        }
        return "";
    }

    bool NitroFile::Read(fs::BinaryFile &base_bf, const size_t offset, void *read_buf, const size_t read_size) const {
        const auto old_offset = base_bf.GetAbsoluteOffset();
        if(!base_bf.SetAbsoluteOffset(this->offset + offset)) {
            return false;
        }
        const auto r_size = std::min(this->size, read_size);
        const auto read_ok = base_bf.ReadData(read_buf, r_size);
        if(!base_bf.SetAbsoluteOffset(old_offset)) {
            return false;
        }
        
        return read_ok;
    }

    bool ReadNitroFsFrom(const size_t fat_data_offset, const size_t fnt_data_offset, fs::BinaryFile &bf, NitroDirectory &out_fs_root_dir) {
        out_fs_root_dir.is_root = true;
        out_fs_root_dir.entry_offset = UINT32_MAX;
        return ReadNitroDirectoryImpl(fat_data_offset, fnt_data_offset, bf, out_fs_root_dir, InitialDirectoryId);
    }

    bool NitroFsFileFormat::LookupFile(const std::string &path, NitroFile &out_file) const {
        auto pos_init = 0;
        auto pos_found = 0;
        auto out_res = false;

        if(!this->DoWithReadFile([&](fs::BinaryFile &bf) {
            const auto *cur_dir = std::addressof(this->nitro_fs);
            auto found = true;
            std::string token = "";
            while(pos_found >= 0) {
                pos_found = path.find('/', pos_init);
                token = path.substr(pos_init, pos_found - pos_init);

                pos_init = pos_found + 1;

                if(!found) {
                    // Last directory was not found, and there is another token left
                    out_res = false;
                    return;
                }

                found = false;
                for(const auto &dir : cur_dir->dirs) {
                    if(token == dir.GetName(bf)) {
                        cur_dir = std::addressof(dir);
                        found = true;
                        break;
                    }
                }
            }

            // If everything went alright and the path is valid, last token must be a file at cur_dir
            for(const auto &file : cur_dir->files) {
                if(token == file.GetName(bf)) {
                    out_file = file;
                    out_res = true;
                    return;
                }
            }
        })) {
            return false;
        }
        
        return out_res;
    }

    std::string NitroFsFileFormat::GetName(const NitroEntryBase &entry) const {
        fs::BinaryFile bf;
        if(bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp)) {
            return entry.GetName(bf);
        }
        return "";
    }

    bool NitroFsFileFormat::SaveFileSystem() {
        std::vector<std::string> ext_fs_files;
        if(!fs::ListAllStdioFiles(this->ext_fs_root_path, ext_fs_files)) {
            return false;
        }

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
                if(!this->LookupFile(nfs_path, nfs_file)) {
                    return false;
                }
                nfs_files.push_back(std::make_pair(ext_fs_file, nfs_file));
            }

            std::sort(nfs_files.begin(), nfs_files.end(), [](const std::pair<std::string, NitroFile> &p_a, const std::pair<std::string, NitroFile> &p_b) -> bool {
                return p_a.second.offset < p_b.second.offset;
            });

            fs::BinaryFile w_bf;
            if(!w_bf.Open(w_file_handle, w_path, fs::OpenMode::Write, this->comp)) {
                return false;
            }

            fs::BinaryFile r_bf;
            if(!r_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp)) {
                return false;
            }

            const auto orig_self_size = r_bf.GetSize();
            const auto base_offset = this->GetBaseOffset();
            const auto pre_file_data_offset = 0;
            const auto pre_file_data_size = base_offset + nfs_files.front().second.offset;
            const auto post_file_data_offset = base_offset + nfs_files.back().second.offset + nfs_files.back().second.size;
            if(!r_bf.SetAbsoluteOffset(pre_file_data_offset)) {
                return false;
            }
            if(!w_bf.CopyFrom(r_bf, pre_file_data_size)) {
                return false;
            }

            const auto fat_entries_offset = this->GetFatEntriesOffset();
            const auto fat_entry_count = this->GetFatEntryCount();
            const auto nfs_file_count = nfs_files.size();
            auto fat_entries = util::NewArray<FileAllocationTableEntry>(fat_entry_count);
            if(!r_bf.SetAbsoluteOffset(fat_entries_offset)) {
                return false;
            }
            if(!r_bf.ReadData(fat_entries, sizeof(FileAllocationTableEntry) * fat_entry_count)) {
                return false;
            }
            ssize_t size_diff = 0;

            u32 i = 0;
            for(auto &[ext_fs_file, nfs_file] : nfs_files) {
                const auto nfs_file_w_offset = nfs_file.offset + size_diff;
                const auto nfs_file_r_offset = nfs_file.offset;
                const auto new_file_size = fs::GetStdioFileSize(ext_fs_file);
                {
                    fs::BinaryFile d_bf;
                    if(!d_bf.Open(std::make_shared<fs::StdioFileHandle>(), ext_fs_file, fs::OpenMode::Read)) {
                        return false;
                    }
                    if(!w_bf.SetAbsoluteOffset(base_offset + nfs_file_w_offset)) {
                        return false;
                    }
                    if(!w_bf.CopyFrom(d_bf, new_file_size)) {
                        return false;
                    }
                }

                size_t pad_count = 0;
                if(this->HasAlignmentBetweenFileData()) {
                    while((w_bf.GetAbsoluteOffset() % 0x200) != 0) {
                        if(!w_bf.Write<u8>(0xff)) {
                            return false;
                        }
                        pad_count++;
                    }
                }

                const auto has_next_file = (i + 1) < nfs_file_count;
                if(has_next_file) {
                    const auto &[next_ext_fs_file, next_nfs_file] = nfs_files[i + 1];
                    const auto data_between_files_offset = base_offset + nfs_file_r_offset + nfs_file.size;
                    auto data_between_files_size = next_nfs_file.offset - nfs_file_r_offset - nfs_file.size;
    
                    if(this->HasAlignmentBetweenFileData()) {
                        auto tmp_align_offset = w_bf.GetAbsoluteOffset() + data_between_files_size;
                        auto data_between_files_pad_count = 0;
                        while((tmp_align_offset % 0x200) != 0) {
                            tmp_align_offset++;
                            data_between_files_pad_count++;
                        }
                        data_between_files_size += data_between_files_pad_count;
                    }
                    
                    if(!r_bf.SetAbsoluteOffset(data_between_files_offset)) {
                        return false;
                    }
                    if(!w_bf.CopyFrom(r_bf, data_between_files_size)) {
                        return false;
                    }
                }

                const ssize_t cur_size_diff = new_file_size + pad_count - nfs_file.size;

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
                // UpdateNitroDirectoryOffsetsImpl(this->nitro_fs, nfs_file, new_file_size, pad_count);

                size_diff += cur_size_diff;
                i++;
            }

            const auto old_offset = w_bf.GetAbsoluteOffset();
            if(!w_bf.SetAbsoluteOffset(fat_entries_offset)) {
                return false;
            }
            if(!w_bf.WriteData(fat_entries, sizeof(FileAllocationTableEntry) * fat_entry_count)) {
                return false;
            }
            delete[] fat_entries;
            if(!w_bf.SetAbsoluteOffset(old_offset)) {
                return false;
            }

            const auto post_file_data_size = orig_self_size - post_file_data_offset;

            if(!r_bf.SetAbsoluteOffset(post_file_data_offset)) {
                return false;
            }

            if(!w_bf.CopyFrom(r_bf, post_file_data_size)) {
                return false;
            }

            /* format-specific final writes */
            if(!this->OnFileSystemWrite(w_bf, size_diff)) {
                return false;
            }
        }
        else {
            fs::BinaryFile w_bf;
            if(!w_bf.Open(w_file_handle, w_path, fs::OpenMode::Write, this->comp)) {
                return false;
            }

            fs::BinaryFile r_bf;
            if(!r_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp)) {
                return false;
            }

            if(!w_bf.CopyFrom(r_bf, r_bf.GetSize())) {
                return false;
            }

            if(!this->OnFileSystemWrite(w_bf, 0)) {
                return false;
            }
        }

        if(write_on_self) {
            fs::BinaryFile w_bf;
            if(!w_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Write)) {
                return false;
            }

            fs::BinaryFile r_bf;
            if(!r_bf.Open(w_file_handle, w_path, fs::OpenMode::Read)) {
                return false;
            }

            if(!w_bf.CopyFrom(r_bf, r_bf.GetSize())) {
                return false;
            }

            if(!fs::DeleteStdioFile(w_path)) {
                return false;
            }

            w_file_handle = nullptr;
            w_path.clear();
        }

        for(const auto &ext_fs_file : ext_fs_files) {
            fs::DeleteStdioFile(ext_fs_file);
        }

        return true;
    }

}