#include <ntr/fmt/fmt_BMG.hpp>
#include <ntr/util/util_String.hpp>
#include <ntr/util/util_Memory.hpp>

namespace ntr::fmt {

    namespace {

        constexpr size_t DataAlignment = 0x10;

    }

    bool BMG::CreateFrom(const Encoding enc, const size_t attr_size, const std::vector<String> &strs, const u32 file_id, const ntr::fs::FileCompression comp) {
        // The rest of the fields will be automatically set when writing
        this->comp = comp;

        this->header.encoding = enc;

        this->info.entry_size = InfoSection::OffsetSize + attr_size;
        this->info.file_id = file_id;
        this->strings = strs;

        return true;
    }

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

        if(GetCharacterSize(this->header.encoding) == 0) {
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

        const auto attrs_size = this->info.entry_size - InfoSection::OffsetSize;
        for(auto i = 0; i < this->info.offset_count; i++) {
            u32 offset = 0;
            if(!bf.Read(offset)) {
                return false;
            }
            String str = {};
            for(u32 j = 0; j < attrs_size; j++) {
                u8 attr;
                if(!bf.Read(attr)) {
                    return false;
                }
                str.attrs.push_back(attr);
            }

            const auto old_offset = bf.GetAbsoluteOffset();

            if(!bf.SetAbsoluteOffset(strings_offset + offset)) {
                return false;
            }

            switch(this->header.encoding) {
                case Encoding::CP1252: {
                    // Unsupported yet
                    return false;
                }
                case Encoding::UTF16: {
                    if(!bf.ReadNullTerminatedString(str.str)) {
                        return false;
                    }
                    break;
                }
                case Encoding::UTF8: {
                    std::string utf8_str;
                    if(!bf.ReadNullTerminatedString(utf8_str)) {
                        return false;
                    }
                    str.str = util::ConvertToUnicode(utf8_str);
                    break;
                }
                case Encoding::ShiftJIS: {
                    // Unsupported yet
                    return false;
                }
            }
            this->strings.push_back(str);

            if(!bf.SetAbsoluteOffset(old_offset)) {
                return false;
            }
        }

        return true;
    }

    bool BMG::WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        // Ensure strings are correct
        const auto attrs_size = this->info.entry_size - InfoSection::OffsetSize;
        for(const auto &str: this->strings) {
            if(str.attrs.size() != attrs_size) {
                return false;
            }
        }

        fs::BinaryFile bf = {};
        if(!bf.Open(file_handle, path, fs::OpenMode::Write, comp)) {
            return false;
        }

        this->info.EnsureMagic();
        this->info.block_size = util::AlignUp(sizeof(InfoSection) + this->strings.size() * this->info.entry_size, DataAlignment);
        this->info.offset_count = this->strings.size();
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
            const auto cur_str = this->strings.at(i);

            if(!bf.Write(cur_offset)) {
                return false;
            }
            if(!bf.WriteVector(cur_str.attrs)) {
                return false;
            }

            const auto info_section_offset = bf.GetAbsoluteOffset();
            if(!bf.SetAbsoluteOffset(strings_offset + cur_offset)) {
                return false;
            }

            switch(this->header.encoding) {
                case Encoding::CP1252: {
                    // Unsupported
                    return false;
                }
                case Encoding::UTF16: {
                    if(!bf.WriteNullTerminatedString(cur_str.str)) {
                        return false;
                    }
                    break;
                }
                case Encoding::UTF8: {
                    const auto utf8_str = util::ConvertFromUnicode(cur_str.str);
                    if(!bf.WriteNullTerminatedString(utf8_str)) {
                        return false;
                    }
                    break;
                }
                case Encoding::ShiftJIS: {
                    // Unsupported
                    return false;
                }
            }

            if(!bf.SetAbsoluteOffset(info_section_offset)) {
                return false;
            }

            cur_offset += cur_str.GetByteLength(this->header.encoding);
        }

        if(!bf.SetAtEnd()) {
            return false;
        }

        while(!util::IsAlignedTo(bf.GetAbsoluteOffset(), DataAlignment)) {
            if(!bf.Write<u8>(0)) {
                return false;
            }
        }

        this->header.EnsureMagic();
        this->header.file_size = bf.GetAbsoluteOffset();
        this->header.section_count = 2;
        if(!bf.SetAbsoluteOffset(0)) {
            return false;
        }
        if(!bf.Write(this->header)) {
            return false;
        }

        this->data.EnsureMagic();
        this->data.block_size = this->header.file_size - data_offset;
        if(!bf.SetAbsoluteOffset(data_offset)) {
            return false;
        }
        if(!bf.Write(this->data)) {
            return false;
        }

        return true;
    }

}