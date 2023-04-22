#include <ntr/fmt/fmt_SDAT.hpp>
#include <ntr/util/util_String.hpp>

namespace ntr::fmt {

    namespace {

        bool ReadSymbolRecord(fs::BinaryFile &bf, const size_t symb_offset, SDAT::SymbolRecord &out_record) {
            if(!bf.Read(out_record.entry_count)) {
                return false;
            }

            out_record.entries.reserve(out_record.entry_count);
            for(u32 i = 0; i < out_record.entry_count; i++) {
                SDAT::SymbolRecordEntry entry = {};
                if(!bf.Read(entry.name_offset)) {
                    return false;
                }

                if(entry.name_offset != 0) {
                    const auto old_offset = bf.GetAbsoluteOffset();
                    if(!bf.SetAbsoluteOffset(symb_offset + entry.name_offset)) {
                        return false;
                    }
                    if(!bf.ReadNullTerminatedString(entry.name)) {
                        return false;
                    }
                    bf.SetAbsoluteOffset(old_offset);
                }

                out_record.entries.push_back(std::move(entry));
            }

            return true;
        }

        bool ReadSequenceArchiveSymbolRecord(fs::BinaryFile &bf, const size_t symb_offset, SDAT::SequenceArchiveSymbolRecord &out_seq_arc_record) {
            if(!bf.Read(out_seq_arc_record.entry_count)) {
                return false;
            }

            out_seq_arc_record.entries.reserve(out_seq_arc_record.entry_count);
            for(u32 i = 0; i < out_seq_arc_record.entry_count; i++) {
                SDAT::SequenceArchiveSymbolRecordEntry entry = {};
                if(!bf.Read(entry.name_offset)) {
                    return false;
                }
                if(!bf.Read(entry.subrecord_offset)) {
                    return false;
                }

                if(entry.name_offset != 0) {
                    const auto old_offset = bf.GetAbsoluteOffset();
                    if(!bf.SetAbsoluteOffset(symb_offset + entry.name_offset)) {
                        return false;
                    }
                    if(!bf.ReadNullTerminatedString(entry.name)) {
                        return false;
                    }
                    bf.SetAbsoluteOffset(old_offset);
                }
                if(entry.subrecord_offset != 0) {
                    const auto old_offset = bf.GetAbsoluteOffset();
                    if(!bf.SetAbsoluteOffset(symb_offset + entry.subrecord_offset)) {
                        return false;
                    }
                    if(!ReadSymbolRecord(bf, symb_offset, entry.subrecord)) {
                        return false;
                    }
                    bf.SetAbsoluteOffset(old_offset);
                }

                out_seq_arc_record.entries.push_back(std::move(entry));
            }

            return true;
        }

        template<typename T>
        bool ReadInfoRecord(fs::BinaryFile &bf, const size_t info_offset, SDAT::InfoRecord<T> &out_record) {
            if(!bf.Read(out_record.entry_count)) {
                return false;
            }

            out_record.entries.reserve(out_record.entry_count);
            for(u32 i = 0; i < out_record.entry_count; i++) {
                SDAT::InfoRecordEntry<T> entry = {};
                if(!bf.Read(entry.info_offset)) {
                    return false;
                }

                if(entry.info_offset != 0) {
                    const auto old_offset = bf.GetAbsoluteOffset();
                    if(!bf.SetAbsoluteOffset(info_offset + entry.info_offset)) {
                        return false;
                    }
                    if(!bf.Read(entry.info)) {
                        return false;
                    }
                    bf.SetAbsoluteOffset(old_offset);
                }

                out_record.entries.push_back(std::move(entry));
            }

            return true;
        }

        template<typename S, typename T>
        bool LocateFileImpl(const bool has_symb, const S &symb_record, const SDAT::InfoRecord<T> &info_rec, const std::string &token, u32 &out_file_id) {
            u32 idx;
            if(util::ConvertStringToNumber(token, idx)) {
                if(idx < info_rec.entry_count) {
                    out_file_id = info_rec.entries[idx].info.file_id;
                    return true;
                }
            }
            else {
                if(!has_symb) {
                    return false;
                }
                for(u32 i = 0; i < symb_record.entry_count; i++) {
                    const auto &entry = symb_record.entries[i];
                    if(entry.name == token) {
                        out_file_id = info_rec.entries[i].info.file_id;
                        return true;
                    }
                }
            }
            return false;
        }

    }

    bool SDAT::LocateFile(const std::string &path, u32 &out_file_id) {
        const auto tokens = util::SplitString(path, '/');
        if(tokens.size() == 2) {
            const auto kind = tokens[0];
            const auto token = tokens[1];
            if(kind == "seq") {
                return LocateFileImpl(this->HasSymbols(), this->seq_symb_record, this->seq_info_record, token, out_file_id);
            }
            if(kind == "seqarc") {
                return LocateFileImpl(this->HasSymbols(), this->seq_arc_symb_record, this->seq_arc_info_record, token, out_file_id);
            }
            if(kind == "bnk") {
                return LocateFileImpl(this->HasSymbols(), this->bnk_symb_record, this->bnk_info_record, token, out_file_id);
            }
            if(kind == "wavarc") {
                return LocateFileImpl(this->HasSymbols(), this->wav_arc_symb_record, this->wav_arc_info_record, token, out_file_id);
            }
            // Unsupported: player, group, stream player
            if(kind == "strm") {
                return LocateFileImpl(this->HasSymbols(), this->strm_symb_record, this->strm_info_record, token, out_file_id);
            }
        }
        return false;
    }

    bool SDAT::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        if(!bf.Open(file_handle, path, fs::OpenMode::Read, comp)) {
            return false;
        }

        if(!bf.Read(this->header)) {
            return false;
        }
        if(!this->header.IsValid()) {
            return false;
        }

        if(this->header.symb_size > 0) {
            if(!bf.SetAbsoluteOffset(this->header.symb_offset)) {
                return false;
            }
            if(!bf.Read(this->symb)) {
                return false;
            }
            if(!this->symb.IsValid()) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.seq_record_offset)) {
                return false;
            }
            if(!ReadSymbolRecord(bf, this->header.symb_offset, this->seq_symb_record)) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.seq_arc_record_offset)) {
                return false;
            }
            if(!ReadSequenceArchiveSymbolRecord(bf, this->header.symb_offset, this->seq_arc_symb_record)) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.bnk_record_offset)) {
                return false;
            }
            if(!ReadSymbolRecord(bf, this->header.symb_offset, this->bnk_symb_record)) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.wav_arc_record_offset)) {
                return false;
            }
            if(!ReadSymbolRecord(bf, this->header.symb_offset, this->wav_arc_symb_record)) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.player_record_offset)) {
                return false;
            }
            if(!ReadSymbolRecord(bf, this->header.symb_offset, this->player_symb_record)) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.group_record_offset)) {
                return false;
            }
            if(!ReadSymbolRecord(bf, this->header.symb_offset, this->group_symb_record)) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.strm_player_record_offset)) {
                return false;
            }
            if(!ReadSymbolRecord(bf, this->header.symb_offset, this->strm_player_symb_record)) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(this->header.symb_offset + this->symb.strm_record_offset)) {
                return false;
            }
            if(!ReadSymbolRecord(bf, this->header.symb_offset, this->strm_symb_record)) {
                return false;
            }
        }

        if(!bf.SetAbsoluteOffset(this->header.info_offset)) {
            return false;
        }
        if(!bf.Read(this->info)) {
            return false;
        }
        if(!this->info.IsValid()) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.info_offset + this->info.seq_record_offset)) {
            return false;
        }
        if(!ReadInfoRecord(bf, this->header.info_offset, this->seq_info_record)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.info_offset + this->info.seq_arc_record_offset)) {
            return false;
        }
        if(!ReadInfoRecord(bf, this->header.info_offset, this->seq_arc_info_record)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.info_offset + this->info.bnk_record_offset)) {
            return false;
        }
        if(!ReadInfoRecord(bf, this->header.info_offset, this->bnk_info_record)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.info_offset + this->info.wav_arc_record_offset)) {
            return false;
        }
        if(!ReadInfoRecord(bf, this->header.info_offset, this->wav_arc_info_record)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.info_offset + this->info.player_record_offset)) {
            return false;
        }
        if(!ReadInfoRecord(bf, this->header.info_offset, this->player_info_record)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.info_offset + this->info.group_record_offset)) {
            return false;
        }
        if(!ReadInfoRecord(bf, this->header.info_offset, this->group_info_record)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.info_offset + this->info.strm_player_record_offset)) {
            return false;
        }
        if(!ReadInfoRecord(bf, this->header.info_offset, this->strm_player_info_record)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.info_offset + this->info.strm_record_offset)) {
            return false;
        }
        if(!ReadInfoRecord(bf, this->header.info_offset, this->strm_info_record)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.fat_offset)) {
            return false;
        }
        if(!bf.Read(this->fat)) {
            return false;
        }
        if(!this->fat.IsValid()) {
            return false;
        }

        this->fat_records.reserve(this->fat.record_count);
        for(u32 i = 0; i < this->fat.record_count; i++) {
            FileAllocationTableRecord record;
            if(!bf.Read(record)) {
                return false;
            }
            this->fat_records.push_back(std::move(record));
        }

        if(!bf.SetAbsoluteOffset(this->header.file_offset)) {
            return false;
        }
        if(!bf.Read(this->file)) {
            return false;
        }
        if(!this->file.IsValid()) {
            return false;
        }

        return true;
    }

    bool SDAT::SaveFileSystem() {
        std::vector<std::string> ext_fs_files;
        if(!fs::ListAllStdioFiles(this->ext_fs_root_path, ext_fs_files)) {
            return false;
        }

        if(!ext_fs_files.empty()) {
            std::vector<std::pair<std::string, FileAllocationTableRecord>> files;
            for(const auto &ext_fs_file : ext_fs_files) {
                const auto base_path = this->GetBasePath(ext_fs_file);
                u32 file_id;
                if(!this->LocateFile(base_path, file_id)) {
                    return false;
                }
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
                if(!w_bf.Open(w_file_handle, w_path, fs::OpenMode::Write, this->comp)) {
                    return false;
                }

                fs::BinaryFile r_bf;
                if(!r_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp)) {
                    return false;
                }

                const auto orig_self_size = r_bf.GetSize();
                const auto pre_file_data_offset = 0;
                const auto pre_file_data_size = files.front().second.offset;
                const auto post_file_data_offset = + files.back().second.offset + files.back().second.size;
                if(!r_bf.SetAbsoluteOffset(pre_file_data_offset)) {
                    return false;
                }
                if(!w_bf.CopyFrom(r_bf, pre_file_data_size)) {
                    return false;
                }

                const auto fat_entries_offset = this->header.fat_offset + sizeof(FileAllocationTableBlock);
                const auto fat_entry_count = this->fat_records.size();
                const auto file_count = files.size();
                ssize_t size_diff = 0;

                u32 i = 0;
                for(auto &[ext_fs_file, file_record] : files) {
                    const auto new_file_size = fs::GetStdioFileSize(ext_fs_file);
                    {
                        fs::BinaryFile d_bf;
                        if(!d_bf.Open(std::make_shared<fs::StdioFileHandle>(), ext_fs_file, fs::OpenMode::Read)) {
                            return false;
                        }
                        if(!w_bf.CopyFrom(d_bf, new_file_size)) {
                            return false;
                        }
                    }

                    size_t pad_count = 0;
                    while((w_bf.GetAbsoluteOffset() % 0x20) != 0) {
                        if(!w_bf.Write<u8>(0)) {
                            return false;
                        }
                        pad_count++;
                    }

                    const auto has_next_file = (i + 1) < file_count;
                    if(has_next_file) {
                        const auto &[next_ext_fs_file, next_file_record] = files[i + 1];
                        const auto data_between_files_offset = file_record.offset + file_record.size;
                        auto data_between_files_size = next_file_record.offset - file_record.offset - file_record.size;
        
                        auto tmp_align_offset = w_bf.GetAbsoluteOffset() + data_between_files_size;
                        auto data_between_files_pad_count = 0;
                        while((tmp_align_offset % 0x20) != 0) {
                            tmp_align_offset++;
                            data_between_files_pad_count++;
                        }
                        data_between_files_size += data_between_files_pad_count;
                        
                        if(!r_bf.SetAbsoluteOffset(data_between_files_offset)) {
                            return false;
                        }
                        if(!w_bf.CopyFrom(r_bf, data_between_files_size)) {
                            return false;
                        }
                    }

                    const ssize_t cur_size_diff = new_file_size + pad_count - file_record.size;

                    const auto old_offset = w_bf.GetAbsoluteOffset();
                    for(size_t i = 0; i < fat_entry_count; i++) {
                        auto &fat_record = this->fat_records[i];
                        const auto cur_entry_offset = fat_entries_offset + i * sizeof(FileAllocationTableRecord);
                        if(!r_bf.SetAbsoluteOffset(cur_entry_offset)) {
                            return false;
                        }

                        if(!w_bf.SetAbsoluteOffset(cur_entry_offset)) {
                            return false;
                        }
                        
                        if(fat_record.offset > file_record.offset) {
                            // It's a file after our current file, advance cur_size_diff the file position
                            fat_record.offset += cur_size_diff;

                            if(!w_bf.Write(fat_record)) {
                                return false;
                            }
                        }
                        else if(fat_record.offset == file_record.offset) {
                            // It's our current file, set the new size
                            fat_record.size = new_file_size;

                            if(!w_bf.Write(fat_record)) {
                                return false;
                            }
                        }
                    }
                    if(!w_bf.SetAbsoluteOffset(old_offset)) {
                        return false;
                    }

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

                if(!r_bf.SetAbsoluteOffset(post_file_data_offset)) {
                    return false;
                }

                if(!w_bf.CopyFrom(r_bf, post_file_data_size)) {
                    return false;
                }

                this->header.file_size += size_diff;
                this->file.block_size += size_diff;
                if(!w_bf.SetAbsoluteOffset(0)) {
                    return false;
                }
                if(!w_bf.Write(this->header)) {
                    return false;
                }
                if(!w_bf.SetAbsoluteOffset(this->header.file_offset)) {
                    return false;
                }
                if(!w_bf.Write(this->file)) {
                    return false;
                }
            }

            if(write_on_self) {
                fs::BinaryFile w_bf;
                if(!w_bf.Open(read_file_handle, read_path, fs::OpenMode::Write)) {
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
        }

        return true;
    }

    bool SDATFileHandle::ExistsImpl(const std::string &path, size_t &out_size) {
        u32 tmp_file_id;
        if(!this->ext_fs_file.LocateFile(path, tmp_file_id)) {
            return false;
        }
        
        out_size = this->ext_fs_file.fat_records[tmp_file_id].size;
        return true;
    }

    bool SDATFileHandle::OpenImpl(const std::string &path) {
        if(!this->ext_fs_file.LocateFile(path, this->file_id)) {
            return false;
        }
        
        if(!this->base_bf.Open(this->ext_fs_file.read_file_handle, this->ext_fs_file.read_path, fs::OpenMode::Read)) {
            return false;
        }

        const auto f_base_offset = this->GetFileOffset();
        if(!this->base_bf.SetAbsoluteOffset(f_base_offset)) {
            return false;
        }

        return true;
    }

    size_t SDATFileHandle::GetSizeImpl() {
        return this->GetFileSize();
    }

    bool SDATFileHandle::SetOffsetImpl(const size_t offset, const fs::Position pos) {
        const auto f_size = this->GetSize();
        const auto f_base_offset = this->GetFileOffset();
        const auto f_cur_offset = this->base_bf.GetAbsoluteOffset();
        switch(pos) {
            case fs::Position::Begin: {
                if(offset > f_size) {
                    return false;
                }
                this->base_bf.SetAbsoluteOffset(f_base_offset + offset);
                return true;
            }
            case fs::Position::Current: {
                if((f_cur_offset + offset) > f_size) {
                    return false;
                }
                this->base_bf.SetAbsoluteOffset(f_base_offset + f_cur_offset + offset);
                return true;
            }
            case fs::Position::End: {
                if(offset != 0) {
                    return false;
                }
                this->base_bf.SetAbsoluteOffset(f_base_offset + f_size);
                return true;
            }
        }
        return false;
    }

    size_t SDATFileHandle::GetOffsetImpl() {
        const auto f_base_offset = this->GetFileOffset();
        return this->base_bf.GetAbsoluteOffset() - f_base_offset;
    }

    bool SDATFileHandle::ReadImpl(void *read_buf, const size_t read_size) {
        const auto f_offset = this->GetOffset();
        const auto f_size = this->GetSize();
        if((f_offset + read_size) > f_size) {
            return false;
        }
        return this->base_bf.ReadData(read_buf, read_size);
    }

    bool SDATFileHandle::CloseImpl() {
        return this->base_bf.Close();
    }

}