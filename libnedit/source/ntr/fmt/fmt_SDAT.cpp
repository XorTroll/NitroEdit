#include <ntr/fmt/fmt_SDAT.hpp>
#include <ntr/util/util_String.hpp>

void Log(const std::string&);

namespace ntr::fmt {

    namespace {

        constexpr size_t DataAlignment = 0x20;

        Result ReadSymbolRecord(fs::BinaryFile &bf, const size_t symb_offset, SDAT::SymbolRecord &out_record) {
            NTR_R_TRY(bf.Read(out_record.entry_count));

            out_record.entries.reserve(out_record.entry_count);
            for(u32 i = 0; i < out_record.entry_count; i++) {
                SDAT::SymbolRecordEntry entry = {};
                NTR_R_TRY(bf.Read(entry.name_offset));

                if(entry.name_offset != 0) {
                    size_t old_offset;
                    NTR_R_TRY(bf.GetAbsoluteOffset(old_offset));
                    NTR_R_TRY(bf.SetAbsoluteOffset(symb_offset + entry.name_offset));
                    NTR_R_TRY(bf.ReadNullTerminatedString(entry.name));
                    NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));
                }

                out_record.entries.push_back(std::move(entry));
            }

            NTR_R_SUCCEED();
        }

        Result ReadSequenceArchiveSymbolRecord(fs::BinaryFile &bf, const size_t symb_offset, SDAT::SequenceArchiveSymbolRecord &out_seq_arc_record) {
            NTR_R_TRY(bf.Read(out_seq_arc_record.entry_count));

            out_seq_arc_record.entries.reserve(out_seq_arc_record.entry_count);
            for(u32 i = 0; i < out_seq_arc_record.entry_count; i++) {
                SDAT::SequenceArchiveSymbolRecordEntry entry = {};
                NTR_R_TRY(bf.Read(entry.name_offset));
                NTR_R_TRY(bf.Read(entry.subrecord_offset));

                if(entry.name_offset != 0) {
                    size_t old_offset;
                    NTR_R_TRY(bf.GetAbsoluteOffset(old_offset));
                    NTR_R_TRY(bf.SetAbsoluteOffset(symb_offset + entry.name_offset));
                    NTR_R_TRY(bf.ReadNullTerminatedString(entry.name));
                    NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));
                }
                if(entry.subrecord_offset != 0) {
                    size_t old_offset;
                    NTR_R_TRY(bf.GetAbsoluteOffset(old_offset));
                    NTR_R_TRY(bf.SetAbsoluteOffset(symb_offset + entry.subrecord_offset));
                    NTR_R_TRY(ReadSymbolRecord(bf, symb_offset, entry.subrecord));
                    NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));
                }

                out_seq_arc_record.entries.push_back(std::move(entry));
            }

            NTR_R_SUCCEED();
        }

        template<typename T>
        Result ReadInfoRecord(fs::BinaryFile &bf, const size_t info_offset, SDAT::InfoRecord<T> &out_record) {
            NTR_R_TRY(bf.Read(out_record.entry_count));

            out_record.entries.reserve(out_record.entry_count);
            for(u32 i = 0; i < out_record.entry_count; i++) {
                SDAT::InfoRecordEntry<T> entry = {};
                NTR_R_TRY(bf.Read(entry.info_offset));

                if(entry.info_offset != 0) {
                    size_t old_offset;
                    NTR_R_TRY(bf.GetAbsoluteOffset(old_offset));
                    NTR_R_TRY(bf.SetAbsoluteOffset(info_offset + entry.info_offset));
                    NTR_R_TRY(bf.Read(entry.info));
                    NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));
                }

                out_record.entries.push_back(std::move(entry));
            }

            NTR_R_SUCCEED();
        }

        template<typename S, typename T>
        Result LocateFileImpl(const bool has_symb, const S &symb_record, const SDAT::InfoRecord<T> &info_rec, const std::string &token, u32 &out_file_id) {
            u32 idx;
            if(util::ConvertStringToNumber(token, idx)) {
                if(idx < info_rec.entry_count) {
                    out_file_id = info_rec.entries[idx].info.file_id;
                    NTR_R_SUCCEED();
                }
            }
            else {
                if(!has_symb) {
                    NTR_R_FAIL(ResultSDATInvalidSymbols);
                }

                for(u32 i = 0; i < symb_record.entry_count; i++) {
                    const auto &entry = symb_record.entries[i];
                    if(entry.name == token) {
                        out_file_id = info_rec.entries[i].info.file_id;
                        NTR_R_SUCCEED();
                    }
                }
            }

            NTR_R_FAIL(ResultSDATEntryNotFound);
        }

    }

    Result SDAT::LocateFile(const std::string &path, u32 &out_file_id) {
        const auto tokens = util::SplitString(path, '/');
        if(tokens.size() == 2) {
            const auto format = tokens[0];
            const auto token = tokens[1];
            if(format == SSEQVirtualDirectoryName) {
                return LocateFileImpl(this->HasSymbols(), this->seq_symb_record, this->seq_info_record, token, out_file_id);
            }
            if(format == SSARVirtualDirectoryName) {
                return LocateFileImpl(this->HasSymbols(), this->seq_arc_symb_record, this->seq_arc_info_record, token, out_file_id);
            }
            if(format == SBNKVirtualDirectoryName) {
                return LocateFileImpl(this->HasSymbols(), this->bnk_symb_record, this->bnk_info_record, token, out_file_id);
            }
            if(format == SWARVirtualDirectoryName) {
                return LocateFileImpl(this->HasSymbols(), this->wav_arc_symb_record, this->wav_arc_info_record, token, out_file_id);
            }
            // Unsupported: player, group, stream player
            if(format == STRMVirtualDirectoryName) {
                return LocateFileImpl(this->HasSymbols(), this->strm_symb_record, this->strm_info_record, token, out_file_id);
            }
        }

        NTR_R_FAIL(ResultSDATInvalidEntryFormat);
    }

    Result SDAT::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultSDATInvalidHeader);
        }

        if(this->header.symb_size > 0) {
            NTR_R_TRY(bf.SetAbsoluteOffset(this->header.symb_offset));
            NTR_R_TRY(bf.Read(this->symb));
            if(!this->symb.IsValid()) {
                NTR_R_FAIL(ResultSDATInvalidSymbolBlock);
            }
        }

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.info_offset));
        NTR_R_TRY(bf.Read(this->info));
        if(!this->info.IsValid()) {
            NTR_R_FAIL(ResultSDATInvalidInfoBlock);
        }

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.fat_offset));
        NTR_R_TRY(bf.Read(this->fat));
        if(!this->fat.IsValid()) {
            NTR_R_FAIL(ResultSDATInvalidFileAllocationTableBlock);
        }

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.file_offset));
        NTR_R_TRY(bf.Read(this->file));
        if(!this->file.IsValid()) {
            NTR_R_FAIL(ResultSDATInvalidFileBlock);
        }

        NTR_R_SUCCEED();
    }
    
    Result SDAT::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        if(this->header.symb_size > 0) {
            NTR_R_TRY(bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.seq_record_offset));
            NTR_R_TRY(ReadSymbolRecord(bf, this->header.symb_offset, this->seq_symb_record));

            NTR_R_TRY(bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.seq_arc_record_offset));
            NTR_R_TRY(ReadSequenceArchiveSymbolRecord(bf, this->header.symb_offset, this->seq_arc_symb_record));

            NTR_R_TRY(bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.bnk_record_offset));
            NTR_R_TRY(ReadSymbolRecord(bf, this->header.symb_offset, this->bnk_symb_record));

            NTR_R_TRY(bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.wav_arc_record_offset));
            NTR_R_TRY(ReadSymbolRecord(bf, this->header.symb_offset, this->wav_arc_symb_record));

            NTR_R_TRY(bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.player_record_offset));
            NTR_R_TRY(ReadSymbolRecord(bf, this->header.symb_offset, this->player_symb_record));

            NTR_R_TRY(bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.group_record_offset));
            NTR_R_TRY(ReadSymbolRecord(bf, this->header.symb_offset, this->group_symb_record));

            NTR_R_TRY(bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.strm_player_record_offset));
            NTR_R_TRY(ReadSymbolRecord(bf, this->header.symb_offset, this->strm_player_symb_record));

            NTR_R_TRY(bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.strm_record_offset));
            NTR_R_TRY(ReadSymbolRecord(bf, this->header.symb_offset, this->strm_symb_record));
        }

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.info_offset + this->info.seq_record_offset));
        NTR_R_TRY(ReadInfoRecord(bf, this->header.info_offset, this->seq_info_record));

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.info_offset + this->info.seq_arc_record_offset));
        NTR_R_TRY(ReadInfoRecord(bf, this->header.info_offset, this->seq_arc_info_record));

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.info_offset + this->info.bnk_record_offset));
        NTR_R_TRY(ReadInfoRecord(bf, this->header.info_offset, this->bnk_info_record));

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.info_offset + this->info.wav_arc_record_offset));
        NTR_R_TRY(ReadInfoRecord(bf, this->header.info_offset, this->wav_arc_info_record));

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.info_offset + this->info.player_record_offset));
        NTR_R_TRY(ReadInfoRecord(bf, this->header.info_offset, this->player_info_record));

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.info_offset + this->info.group_record_offset));
        NTR_R_TRY(ReadInfoRecord(bf, this->header.info_offset, this->group_info_record));

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.info_offset + this->info.strm_player_record_offset));
        NTR_R_TRY(ReadInfoRecord(bf, this->header.info_offset, this->strm_player_info_record));

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.info_offset + this->info.strm_record_offset));
        NTR_R_TRY(ReadInfoRecord(bf, this->header.info_offset, this->strm_info_record));

        NTR_R_TRY(bf.SetAbsoluteOffset(this->header.fat_offset + sizeof(FileAllocationTableBlock)));
        this->fat_records.reserve(this->fat.record_count);
        for(u32 i = 0; i < this->fat.record_count; i++) {
            FileAllocationTableRecord record;
            NTR_R_TRY(bf.Read(record));
            this->fat_records.push_back(std::move(record));
        }

        NTR_R_SUCCEED();
    }

    Result SDAT::SaveFileSystem() {
        std::vector<std::string> ext_fs_files;
        NTR_R_TRY(fs::ListAllStdioFiles(this->ext_fs_root_path, ext_fs_files));

        if(!ext_fs_files.empty()) {
            std::vector<std::pair<std::string, FileAllocationTableRecord>> files;
            for(const auto &ext_fs_file : ext_fs_files) {
                const auto base_path = this->GetBasePath(ext_fs_file);
                u32 file_id;
                NTR_R_TRY(this->LocateFile(base_path, file_id));
                files.push_back(std::make_pair(ext_fs_file, this->fat_records[file_id]));
            }

            std::sort(files.begin(), files.end(), [](const std::pair<std::string, FileAllocationTableRecord> &p_a, const std::pair<std::string, FileAllocationTableRecord> &p_b) -> bool {
                return p_a.second.offset < p_b.second.offset;
            });

            auto w_path = this->write_path;
            auto w_file_handle = this->write_file_handle;

            const auto write_on_self = (w_file_handle == nullptr) || w_path.empty();
            if(write_on_self) {
                w_file_handle = std::make_shared<fs::StdioFileHandle>();
                w_path = this->ext_fs_root_path + "_tmp_" + fs::GetFileName(this->read_path);
            }

            {
                fs::BinaryFile w_bf;
                NTR_R_TRY(w_bf.Open(w_file_handle, w_path, fs::OpenMode::Write, this->comp));

                fs::BinaryFile r_bf;
                NTR_R_TRY(r_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp));

                size_t orig_self_size;
                NTR_R_TRY(r_bf.GetSize(orig_self_size));
                const auto pre_file_data_offset = 0;
                const auto pre_file_data_size = files.front().second.offset;
                const auto post_file_data_offset = + files.back().second.offset + files.back().second.size;
                NTR_R_TRY(r_bf.SetAbsoluteOffset(pre_file_data_offset));
                NTR_R_TRY(w_bf.CopyFrom(r_bf, pre_file_data_size));

                const auto fat_entries_offset = this->header.fat_offset + sizeof(FileAllocationTableBlock);
                const auto fat_entry_count = this->fat_records.size();
                const auto file_count = files.size();
                ssize_t size_diff = 0;

                u32 i = 0;
                for(auto &[ext_fs_file, file_record] : files) {
                    size_t new_file_size;
                    NTR_R_TRY(fs::GetStdioFileSize(ext_fs_file, new_file_size));
                    {
                        fs::BinaryFile d_bf;
                        NTR_R_TRY(d_bf.Open(std::make_shared<fs::StdioFileHandle>(), ext_fs_file, fs::OpenMode::Read));
                        NTR_R_TRY(w_bf.CopyFrom(d_bf, new_file_size));
                    }

                    size_t pad_size;
                    NTR_R_TRY(w_bf.WriteEnsureAlignment(DataAlignment, pad_size));

                    const auto has_next_file = (i + 1) < file_count;
                    if(has_next_file) {
                        const auto &[next_ext_fs_file, next_file_record] = files[i + 1];
                        const auto data_between_files_offset = file_record.offset + file_record.size;
                        auto data_between_files_size = next_file_record.offset - file_record.offset - file_record.size;
        
                        size_t w_bf_abs_offset;
                        NTR_R_TRY(w_bf.GetAbsoluteOffset(w_bf_abs_offset));
                        auto tmp_align_offset = w_bf_abs_offset + data_between_files_size;
                        auto data_between_files_pad_size = 0;
                        while(!util::IsAlignedTo(tmp_align_offset, DataAlignment)) {
                            tmp_align_offset++;
                            data_between_files_pad_size++;
                        }
                        data_between_files_size += data_between_files_pad_size;
                        
                        NTR_R_TRY(r_bf.SetAbsoluteOffset(data_between_files_offset));
                        NTR_R_TRY(w_bf.CopyFrom(r_bf, data_between_files_size));
                    }

                    const ssize_t cur_size_diff = new_file_size + pad_size - file_record.size;

                    size_t old_offset;
                    NTR_R_TRY(w_bf.GetAbsoluteOffset(old_offset));
                    for(size_t i = 0; i < fat_entry_count; i++) {
                        auto &fat_record = this->fat_records[i];
                        const auto cur_entry_offset = fat_entries_offset + i * sizeof(FileAllocationTableRecord);
                        NTR_R_TRY(r_bf.SetAbsoluteOffset(cur_entry_offset));
                        NTR_R_TRY(w_bf.SetAbsoluteOffset(cur_entry_offset));
                        
                        if(fat_record.offset > file_record.offset) {
                            // It's a file after our current file, advance cur_size_diff the file position
                            fat_record.offset += cur_size_diff;

                            NTR_R_TRY(w_bf.Write(fat_record));
                        }
                        else if(fat_record.offset == file_record.offset) {
                            // It's our current file, set the new size
                            fat_record.size = new_file_size;

                            NTR_R_TRY(w_bf.Write(fat_record));
                        }
                    }
                    NTR_R_TRY(w_bf.SetAbsoluteOffset(old_offset));

                    for(auto &[_ext_fs_file_2, file_record_2] : files) {
                        if(file_record_2.offset > file_record.offset) {
                            file_record_2.offset += cur_size_diff;
                        }
                    }
                    file_record.size = new_file_size;

                    size_diff += cur_size_diff;
                    i++;
                }

                const auto post_file_data_size = orig_self_size - post_file_data_offset;

                NTR_R_TRY(r_bf.SetAbsoluteOffset(post_file_data_offset));

                NTR_R_TRY(w_bf.CopyFrom(r_bf, post_file_data_size));

                this->header.file_size += size_diff;
                this->file.block_size += size_diff;
                NTR_R_TRY(w_bf.SetAbsoluteOffset(0));
                NTR_R_TRY(w_bf.Write(this->header));
                NTR_R_TRY(w_bf.SetAbsoluteOffset(this->header.file_offset));
                NTR_R_TRY(w_bf.Write(this->file));
            }

            if(write_on_self) {
                fs::BinaryFile w_bf;
                NTR_R_TRY(w_bf.Open(read_file_handle, read_path, fs::OpenMode::Write));

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
        }

        NTR_R_SUCCEED();
    }

    bool SDATFileHandle::ExistsImpl(const std::string &path, size_t &out_size) {
        u32 file_id;
        if(this->ext_fs_file->LocateFile(path, file_id).IsSuccess()) {
            out_size = this->ext_fs_file->fat_records[file_id].size;
            return true;    
        }
        else {
            return false;
        }
    }

    Result SDATFileHandle::OpenImpl(const std::string &path) {
        NTR_R_TRY(this->ext_fs_file->LocateFile(path, this->file_id));
        
        NTR_R_TRY(this->base_bf.Open(this->ext_fs_file->read_file_handle, this->ext_fs_file->read_path, fs::OpenMode::Read));

        const auto f_base_offset = this->GetFileOffset();
        NTR_R_TRY(this->base_bf.SetAbsoluteOffset(f_base_offset));

        NTR_R_SUCCEED();
    }

    Result SDATFileHandle::GetSizeImpl(size_t &out_size) {
        out_size = this->GetFileSize();
        NTR_R_SUCCEED();
    }

    Result SDATFileHandle::SetOffsetImpl(const size_t offset, const fs::Position pos) {
        size_t f_size;
        NTR_R_TRY(this->GetSize(f_size));

        const auto f_base_offset = this->GetFileOffset();
        size_t f_cur_offset;
        NTR_R_TRY(this->base_bf.GetAbsoluteOffset(f_cur_offset));

        switch(pos) {
            case fs::Position::Begin: {
                if(offset > f_size) {
                    NTR_R_FAIL(ResultEndOfData);
                }
                else {
                    return this->base_bf.SetAbsoluteOffset(f_base_offset + offset);
                }
            }
            case fs::Position::Current: {
                if((f_cur_offset + offset) > f_size) {
                    NTR_R_FAIL(ResultEndOfData);
                }
                else {
                    return this->base_bf.SetAbsoluteOffset(f_base_offset + f_cur_offset + offset);
                }
            }
            default: {
                NTR_R_FAIL(ResultInvalidSeekPosition);
            }
        }
    }

    Result SDATFileHandle::GetOffsetImpl(size_t &out_offset) {
        const auto f_base_offset = this->GetFileOffset();
        size_t f_cur_offset;
        NTR_R_TRY(this->base_bf.GetAbsoluteOffset(f_cur_offset));

        out_offset = f_cur_offset - f_base_offset;
        NTR_R_SUCCEED();
    }

    Result SDATFileHandle::ReadImpl(void *read_buf, const size_t read_size, size_t &out_read_size) {
        size_t f_offset;
        NTR_R_TRY(this->GetOffset(f_offset));
        size_t f_size;
        NTR_R_TRY(this->GetSize(f_size));
    
        auto actual_read_size = read_size;
        if((f_offset + actual_read_size) > f_size) {
            actual_read_size = f_size - f_offset;
        }

        if(actual_read_size > 0) {
            return this->base_bf.ReadData(read_buf, actual_read_size, out_read_size);
        }
        else {
            NTR_R_FAIL(ResultEndOfData);
        }
    }

    Result SDATFileHandle::CloseImpl() {
        return this->base_bf.Close();
    }

}