#include <ntr/fmt/fmt_BMG.hpp>
#include <ntr/util/util_String.hpp>
#include <ntr/util/util_Memory.hpp>

namespace ntr::fmt {

    namespace {

        constexpr size_t DataAlignment = 0x20;
        constexpr char16_t EscapeCharacter = '\u001A';

        inline Result ReadCharacter(fs::BinaryFile &bf, const BMG::Encoding enc, char16_t &out_ch) {
            switch(enc) {
                case BMG::Encoding::CP1252: {
                    // TODO: unsupported yet
                    NTR_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
                }
                case BMG::Encoding::UTF16: {
                    NTR_R_TRY(bf.Read(out_ch));
                    break;
                }
                case BMG::Encoding::UTF8: {
                    char ch;
                    NTR_R_TRY(bf.Read(ch));
                    out_ch = ch;
                    break;
                }
                case BMG::Encoding::ShiftJIS: {
                    // TODO: unsupported yet
                    NTR_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
                }
            }

            NTR_R_SUCCEED();
        }

        inline Result WriteCharacter(fs::BinaryFile &bf, const BMG::Encoding enc, const char16_t &ch) {
            switch(enc) {
                case BMG::Encoding::CP1252: {
                    // TODO: unsupported yet
                    NTR_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
                }
                case BMG::Encoding::UTF16: {
                    NTR_R_TRY(bf.Write(ch));
                    break;
                }
                case BMG::Encoding::UTF8: {
                    NTR_R_TRY(bf.Write((char)ch));
                    break;
                }
                case BMG::Encoding::ShiftJIS: {
                    // TODO: unsupported yet
                    NTR_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
                }
            }

            NTR_R_SUCCEED();
        }

    }

    Result BMG::CreateFrom(const Encoding enc, const bool has_message_ids, const size_t attr_size, const std::vector<Message> &msgs, const u32 file_id, const ntr::fs::FileCompression comp) {
        // The rest of the fields will be automatically set when writing
        this->comp = comp;

        this->header.encoding = enc;
        this->header.unk_1 = 0;
        this->header.unk_2 = 0;
        this->header.unk_3 = 0;
        this->header.unk_4 = 0;
        this->header.unk_5 = 0;

        this->info.SetAttributesSize(attr_size);
        this->info.file_id = file_id;
        this->messages = msgs;

        if(has_message_ids) {
            this->msg_id.emplace();
        }
        else {
            this->msg_id = {};
        }

        NTR_R_SUCCEED();
    }

    Result BMG::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultBMGInvalidHeader);
        }
        if(this->header.section_count < 2) {
            NTR_R_FAIL(ResultBMGUnexpectedSectionCount);
        }

        NTR_R_TRY(bf.Read(this->info));
        if(!this->info.IsValid()) {
            NTR_R_FAIL(ResultBMGInvalidInfoSection);
        }

        if(!IsValidEncoding(this->header.encoding)) {
            NTR_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
        }
        
        const auto data_offset = sizeof(Header) + this->info.block_size;
        NTR_R_TRY(bf.SetAbsoluteOffset(data_offset));
        NTR_R_TRY(bf.Read(this->data));
        if(!this->data.IsValid()) {
            NTR_R_FAIL(ResultBMGInvalidDataSection);
        }

        if(this->header.section_count >= 3) {
            const auto msg_id_offset = sizeof(Header) + this->info.block_size + this->data.block_size;
            NTR_R_TRY(bf.SetAbsoluteOffset(msg_id_offset));

            MessageIdSection msg_id;
            NTR_R_TRY(bf.Read(msg_id));
            if(!msg_id.IsValid()) {
                NTR_R_FAIL(ResultBMGInvalidMessageIdSection);
            }

            this->msg_id = msg_id;
        }

        NTR_R_SUCCEED();
    }
    
    Result BMG::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        this->messages.clear();

        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));
        
        const auto data_offset = sizeof(Header) + this->info.block_size;
        const auto entries_offset = sizeof(Header) + sizeof(InfoSection);
        const auto messages_offset = data_offset + sizeof(DataSection);

        this->messages.reserve(this->info.entry_count);
        NTR_R_TRY(bf.SetAbsoluteOffset(entries_offset));

        const auto attrs_size = this->info.entry_size - InfoSection::OffsetSize;
        for(auto i = 0; i < this->info.entry_count; i++) {
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

            NTR_R_TRY(bf.SetAbsoluteOffset(messages_offset + offset));

            MessageToken cur_token = {};
            while(true) {
                char16_t ch;
                NTR_R_TRY(ReadCharacter(bf, this->header.encoding, ch));

                if(ch == EscapeCharacter) {
                    if(cur_token.text.length() > 0) {
                        cur_token.type = MessageTokenType::Text;
                        msg.msg.push_back(cur_token);
                    }
                    else if(cur_token.escape.esc_data.size() > 0) {
                        // No unfinished escapes should exist
                        NTR_R_FAIL(ResultBMGInvalidEscapeSequence);
                    }

                    cur_token = {
                        .type = MessageTokenType::Escape
                    };

                    u8 escape_size;
                    NTR_R_TRY(bf.Read(escape_size));
                    escape_size -= GetCharacterSize(this->header.encoding);
                    escape_size -= sizeof(u8);

                    cur_token.escape.esc_data.reserve(escape_size);
                    for(u32 i = 0; i < escape_size; i++) {
                        u8 escape_byte;
                        NTR_R_TRY(bf.Read(escape_byte));

                        cur_token.escape.esc_data.push_back(escape_byte);
                    }

                    msg.msg.push_back(cur_token);
                    cur_token = {};
                }
                else if(ch == '\u0000') {
                    if(cur_token.text.length() > 0) {
                        cur_token.type = MessageTokenType::Text;
                        msg.msg.push_back(cur_token);
                    }
                    else if(cur_token.escape.esc_data.size() > 0) {
                        // No unfinished escapes should exist
                        NTR_R_FAIL(ResultBMGInvalidEscapeSequence);
                    }

                    break;
                }
                else {
                    if(cur_token.escape.esc_data.size() > 0) {
                        // No unfinished escapes should exist
                        NTR_R_FAIL(ResultBMGInvalidEscapeSequence);
                    }

                    cur_token.text.push_back(ch);
                }
            }

            this->messages.push_back(msg);

            NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));
        }

        if(this->HasMessageIds()) {
            const auto msg_id_offset = sizeof(Header) + this->info.block_size + this->data.block_size;
            const auto ids_offset = msg_id_offset + sizeof(MessageIdSection);
            NTR_R_TRY(bf.SetAbsoluteOffset(ids_offset));

            for(u32 i = 0; i < this->msg_id->id_count; i++) {
                u32 id;
                NTR_R_TRY(bf.Read(id));

                if(i < this->messages.size()) {
                    this->messages.at(i).id = id;
                }
            }
        }

        NTR_R_SUCCEED();
    }

    Result BMG::WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        // Ensure message attributes are correct
        const auto attrs_size = this->info.entry_size - InfoSection::OffsetSize;
        for(const auto &msg: this->messages) {
            if(msg.attrs.size() != attrs_size) {
                NTR_R_FAIL(ResultBMGInvalidMessageAttributeSize);
            }
        }

        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Write, comp));

        const auto info_offset = sizeof(Header);
        const auto entries_offset = info_offset + sizeof(InfoSection);
        const auto entries_size = this->messages.size() * this->info.entry_size;
        this->info.EnsureMagic();
        this->info.entry_count = this->messages.size();
        this->info.block_size = util::AlignUp(sizeof(InfoSection) + entries_size, DataAlignment);
        NTR_R_TRY(bf.SetAbsoluteOffset(info_offset));
        NTR_R_TRY(bf.Write(this->info));

        const auto data_offset = info_offset + this->info.block_size;

        NTR_R_TRY(bf.SetAbsoluteOffset(entries_offset + entries_size));

        size_t info_pad_size;
        NTR_R_TRY(bf.WriteEnsureAlignment(DataAlignment, info_pad_size));

        const auto messages_offset = data_offset + sizeof(DataSection);
        u32 cur_message_rel_offset = GetCharacterSize(this->header.encoding); // For some reason, all BMGs apparently leave an unused null character at the start of the section...
        for(auto i = 0; i < this->info.entry_count; i++) {
            const auto cur_msg = this->messages.at(i);

            NTR_R_TRY(bf.SetAbsoluteOffset(entries_offset + i * this->info.entry_size));
            NTR_R_TRY(bf.Write(cur_message_rel_offset));
            NTR_R_TRY(bf.WriteVector(cur_msg.attrs));

            NTR_R_TRY(bf.SetAbsoluteOffset(messages_offset + cur_message_rel_offset));
            for(const auto &token: cur_msg.msg) {
                switch(token.type) {
                    case MessageTokenType::Escape: {
                        NTR_R_TRY(WriteCharacter(bf, this->header.encoding, EscapeCharacter));
                        NTR_R_TRY(bf.Write(static_cast<u8>(token.GetByteLength(this->header.encoding))));

                        NTR_R_TRY(bf.WriteVector(token.escape.esc_data));
                        break;
                    }
                    case MessageTokenType::Text: {
                        for(const auto &ch: token.text) {
                            NTR_R_TRY(WriteCharacter(bf, this->header.encoding, ch));
                        }
                        break;
                    }
                }
            }
            NTR_R_TRY(WriteCharacter(bf, this->header.encoding, '\u0000'));

            size_t end_offset;
            NTR_R_TRY(bf.GetAbsoluteOffset(end_offset));

            cur_message_rel_offset = end_offset - messages_offset;
        }

        size_t data_pad_size;
        NTR_R_TRY(bf.WriteEnsureAlignment(DataAlignment, data_pad_size));

        const auto data_end_offset = messages_offset + cur_message_rel_offset + data_pad_size;

        if(this->HasMessageIds()) {
            const auto msg_id_offset = data_end_offset;
            const auto ids_offset = msg_id_offset + sizeof(MessageIdSection);

            NTR_R_TRY(bf.SetAbsoluteOffset(ids_offset));
            for(const auto &msg: this->messages) {
                NTR_R_TRY(bf.Write(msg.id));
            }

            size_t msg_id_pad_size;
            NTR_R_TRY(bf.WriteEnsureAlignment(DataAlignment, msg_id_pad_size));
        }

        size_t cur_file_size;
        NTR_R_TRY(bf.GetAbsoluteOffset(cur_file_size));

        this->header.EnsureMagic();
        this->header.file_size = cur_file_size;
        
        this->header.section_count = 2;
        if(this->HasMessageIds()) {
            this->header.section_count++;
        }

        NTR_R_TRY(bf.SetAbsoluteOffset(0));
        NTR_R_TRY(bf.Write(this->header));

        this->data.EnsureMagic();
        this->data.block_size = data_end_offset - data_offset;
        NTR_R_TRY(bf.SetAbsoluteOffset(data_offset));
        NTR_R_TRY(bf.Write(this->data));

        if(this->HasMessageIds()) {
            this->msg_id->EnsureMagic();
            this->msg_id->id_count = this->messages.size();   

            const auto msg_id_offset = sizeof(Header) + this->info.block_size + this->data.block_size;
            NTR_R_TRY(bf.SetAbsoluteOffset(msg_id_offset));
            NTR_R_TRY(bf.Write(*this->msg_id));
        }

        NTR_R_SUCCEED();
    }

}