#include <ntr/fmt/fmt_BMG.hpp>
#include <ntr/util/util_String.hpp>
#include <ntr/util/util_Memory.hpp>

namespace ntr::fmt {

    namespace {

        constexpr size_t DataAlignment = 0x20;

    }

    Result BMG::CreateFrom(const Encoding enc, const size_t attr_size, const std::vector<Message> &msgs, const u32 file_id, const ntr::fs::FileCompression comp) {
        // The rest of the fields will be automatically set when writing
        this->comp = comp;

        this->header.encoding = enc;

        this->info.entry_size = InfoSection::OffsetSize + attr_size;
        this->info.file_id = file_id;
        this->messages = msgs;

        NTR_R_SUCCEED();
    }

    Result BMG::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultBMGInvalidHeader);
        }

        NTR_R_TRY(bf.Read(this->info));
        if(!this->info.IsValid()) {
            NTR_R_FAIL(ResultBMGInvalidInfoSection);
        }

        if(GetCharacterSize(this->header.encoding) == 0) {
            NTR_R_FAIL(ResultBMGInvalidUnsupportedCharacterFormat);
        }
        
        const auto data_offset = sizeof(Header) + this->info.block_size;
        NTR_R_TRY(bf.SetAbsoluteOffset(data_offset));
        NTR_R_TRY(bf.Read(this->data));
        if(!this->data.IsValid()) {
            NTR_R_FAIL(ResultBMGInvalidDataSection);
        }

        NTR_R_SUCCEED();
    }
    
    Result BMG::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        this->messages.clear();

        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));
        
        const auto data_offset = sizeof(Header) + this->info.block_size;
        const auto offsets_offset = sizeof(Header) + sizeof(InfoSection);
        const auto strings_offset = data_offset + sizeof(DataSection);

        this->messages.reserve(this->info.offset_count);
        NTR_R_TRY(bf.SetAbsoluteOffset(offsets_offset));

        const auto attrs_size = this->info.entry_size - InfoSection::OffsetSize;
        for(auto i = 0; i < this->info.offset_count; i++) {
            u32 offset = 0;
            NTR_R_TRY(bf.Read(offset));

            Message msg = {};
            for(u32 j = 0; j < attrs_size; j++) {
                u8 attr;
                NTR_R_TRY(bf.Read(attr));

                msg.attrs.push_back(attr);
            }

            size_t old_offset;
            NTR_R_TRY(bf.GetAbsoluteOffset(old_offset));

            NTR_R_TRY(bf.SetAbsoluteOffset(strings_offset + offset));

            // TODO: pay attention to escaping, special characters used for formatting, etc
            switch(this->header.encoding) {
                case Encoding::CP1252: {
                    // TODO: unsupported yet
                    NTR_R_FAIL(ResultBMGInvalidUnsupportedCharacterFormat);
                }
                case Encoding::UTF16: {
                    NTR_R_TRY(bf.ReadNullTerminatedString(msg.msg_str));
                    break;
                }
                case Encoding::UTF8: {
                    std::string utf8_str;
                    NTR_R_TRY(bf.ReadNullTerminatedString(utf8_str));
                    msg.msg_str = util::ConvertToUnicode(utf8_str);
                    break;
                }
                case Encoding::ShiftJIS: {
                    // TODO: unsupported yet
                    NTR_R_FAIL(ResultBMGInvalidUnsupportedCharacterFormat);
                }
            }
            this->messages.push_back(msg);

            NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));
        }

        NTR_R_SUCCEED();
    }

    Result BMG::WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        // Ensure strings are correct
        const auto attrs_size = this->info.entry_size - InfoSection::OffsetSize;
        for(const auto &msg: this->messages) {
            if(msg.attrs.size() != attrs_size) {
                NTR_R_FAIL(ResultBMGInvalidMessageAttributes);
            }
        }

        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Write, comp));

        this->info.EnsureMagic();
        this->info.block_size = util::AlignUp(sizeof(InfoSection) + this->messages.size() * this->info.entry_size, DataAlignment);
        this->info.offset_count = this->messages.size();
        NTR_R_TRY(bf.SetAbsoluteOffset(sizeof(Header)));
        NTR_R_TRY(bf.Write(this->info));

        const auto data_offset = sizeof(Header) + this->info.block_size;
        const auto offsets_offset = sizeof(Header) + sizeof(InfoSection);
        const auto strings_offset = data_offset + sizeof(DataSection);

        NTR_R_TRY(bf.SetAbsoluteOffset(offsets_offset));

        u32 cur_offset = 0;
        for(auto i = 0; i < this->info.offset_count; i++) {
            const auto cur_msg = this->messages.at(i);

            NTR_R_TRY(bf.Write(cur_offset));
            NTR_R_TRY(bf.WriteVector(cur_msg.attrs));

            size_t info_section_offset;
            NTR_R_TRY(bf.GetAbsoluteOffset(info_section_offset));
            NTR_R_TRY(bf.SetAbsoluteOffset(strings_offset + cur_offset));

            switch(this->header.encoding) {
                case Encoding::CP1252: {
                    // Unsupported
                    NTR_R_FAIL(ResultBMGInvalidUnsupportedCharacterFormat);
                }
                case Encoding::UTF16: {
                    NTR_R_TRY(bf.WriteNullTerminatedString(cur_msg.msg_str));
                    break;
                }
                case Encoding::UTF8: {
                    const auto utf8_str = util::ConvertFromUnicode(cur_msg.msg_str);
                    NTR_R_TRY(bf.WriteNullTerminatedString(utf8_str));
                    break;
                }
                case Encoding::ShiftJIS: {
                    // Unsupported
                    NTR_R_FAIL(ResultBMGInvalidUnsupportedCharacterFormat);
                }
            }

            NTR_R_TRY(bf.SetAbsoluteOffset(info_section_offset));

            cur_offset += cur_msg.GetByteLength(this->header.encoding);
        }

        NTR_R_TRY(bf.SetAtEnd());

        size_t cur_file_size;
        NTR_R_TRY(bf.GetAbsoluteOffset(cur_file_size));

        size_t pad_size;
        NTR_R_TRY(bf.WriteEnsureAlignment(DataAlignment, pad_size));

        this->header.EnsureMagic();
        this->header.file_size = cur_file_size + pad_size;
        this->header.section_count = 2;
        NTR_R_TRY(bf.SetAbsoluteOffset(0));
        NTR_R_TRY(bf.Write(this->header));

        this->data.EnsureMagic();
        this->data.block_size = this->header.file_size - data_offset;
        NTR_R_TRY(bf.SetAbsoluteOffset(data_offset));
        NTR_R_TRY(bf.Write(this->data));

        NTR_R_SUCCEED();
    }

}