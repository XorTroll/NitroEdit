#include <ntr/fmt/fmt_BMG.hpp>

namespace ntr::fmt {

    bool BMG::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        this->strings.clear();

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

        if(!bf.Read(this->info)) {
            return false;
        }
        if(!this->info.IsValid()) {
            return false;
        }
        
        const auto data_offset = sizeof(Header) + this->info.block_size;
        const auto offsets_offset = sizeof(Header) + sizeof(InfoSection);
        const auto strings_offset = data_offset + sizeof(DataSection);

        if(!bf.SetAbsoluteOffset(data_offset)) {
            return false;
        }
        if(!bf.Read(this->data)) {
            return false;
        }
        if(!this->data.IsValid()) {
            return false;
        }

        this->strings.reserve(this->info.offset_count);
        if(!bf.SetAbsoluteOffset(offsets_offset)) {
            return false;
        }
        for(auto i = 0; i < this->info.offset_count; i++) {
            u32 offset = 0;
            if(!bf.Read(offset)) {
                return false;
            }

            const auto old_offset = bf.GetAbsoluteOffset();

            if(!bf.SetAbsoluteOffset(strings_offset + offset)) {
                return false;
            }
            std::u16string cur_str;
            if(!bf.ReadNullTerminatedString(cur_str)) {
                return false;
            }
            this->strings.push_back(cur_str);

            if(!bf.SetAbsoluteOffset(old_offset)) {
                return false;
            }
        }

        return true;
    }

    bool BMG::WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        if(!bf.Open(file_handle, path, fs::OpenMode::Write, comp)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(sizeof(Header))) {
            return false;
        }

        if(!bf.Write(this->info)) {
            return false;
        }

        const auto data_offset = sizeof(Header) + this->info.block_size;
        const auto offsets_offset = sizeof(Header) + sizeof(InfoSection);
        const auto strings_offset = data_offset + sizeof(DataSection);

        if(!bf.SetAbsoluteOffset(offsets_offset)) {
            return false;
        }

        u32 cur_offset = 0;
        for(auto i = 0; i < this->info.offset_count; i++) {
            if(!bf.Write(cur_offset)) {
                return false;
            }
            const auto old_offset = bf.GetAbsoluteOffset();

            if(!bf.SetAbsoluteOffset(strings_offset + cur_offset)) {
                return false;
            }
            const auto cur_string = this->strings[i];
            if(!bf.WriteNullTerminatedString(cur_string)) {
                return false;
            }

            if(!bf.SetAbsoluteOffset(old_offset)) {
                return false;
            }

            cur_offset += (cur_string.length() + 1) * sizeof(char16_t);
        }

        if(!bf.SetAtEnd()) {
            return false;
        }

        while((bf.GetAbsoluteOffset() % 0x10) != 0) {
            if(!bf.Write(static_cast<u8>(0))) {
                return false;
            }
        }

        this->header.file_size = bf.GetAbsoluteOffset();
        this->data.block_size = this->header.file_size - data_offset;

        if(!bf.SetAbsoluteOffset(0)) {
            return false;
        }
        if(!bf.Write(this->header)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(data_offset)) {
            return false;
        }
        if(!bf.Write(this->data)) {
            return false;
        }

        return true;
    }

}