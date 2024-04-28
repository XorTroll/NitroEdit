
#pragma once
#include <ntr/fmt/fmt_Common.hpp>

namespace ntr::fmt {

    struct BMG : public fs::FileFormat {

        enum class Encoding : u8 {
            CP1252 = 1,
            UTF16 = 2,
            ShiftJIS = 3,
            UTF8 = 4
        };

        static inline constexpr size_t GetCharacterSize(const Encoding enc) {
            switch(enc) {
                case Encoding::CP1252: {
                    // Unsupported anyway
                    return 1;
                }
                case Encoding::UTF16: {
                    return sizeof(char16_t);
                }
                case Encoding::UTF8: {
                    return sizeof(char);
                }
                case Encoding::ShiftJIS: {
                    // Unsupported anyway
                    return 1;
                }
            }

            // Default for unknown encodings
            return 0;
        }

        static inline constexpr bool IsValidEncoding(const Encoding enc) {
            return GetCharacterSize(enc) > 0;
        }

        struct Header : public MagicStartBase<u64, 0x31676D624753454D /* "MESGbmg1" */ > {
            u32 file_size;
            u32 section_count;
            Encoding encoding;
            u8 unk_1;
            u16 unk_2;
            u32 unk_3;
            u32 unk_4;
            u32 unk_5;
        };

        struct InfoSection : public CommonBlock<0x31464E49 /* "INF1" */ > {
            u16 entry_count;
            u16 entry_size;
            u32 file_id;

            static constexpr size_t OffsetSize = sizeof(u32);

            inline size_t GetAttributesSize() {
                return this->entry_size - OffsetSize;
            }

            inline void SetAttributesSize(const size_t size) {
                this->entry_size = OffsetSize + size;
            }
        };

        struct DataSection : public CommonBlock<0x31544144 /* "DAT1" */ > {
        };

        struct MessageIdSection : public CommonBlock<0x3144494D /* "MID1" */ > {
            u16 id_count;
            u8 unk_1;
            u8 unk_2;
            u32 unk_3;
        };

        enum class MessageTokenType : u8 {
            Text,
            Escape
        };

        struct MessageEscape {
            std::vector<u8> esc_data;
        };

        struct MessageToken {
            MessageTokenType type;
            std::u16string text;
            MessageEscape escape;

            inline size_t GetByteLength(const Encoding enc) const {
                switch(this->type) {
                    case MessageTokenType::Escape: {
                        return GetCharacterSize(enc) + 1 + this->escape.esc_data.size();
                    }
                    case MessageTokenType::Text: {
                        return this->text.length() * GetCharacterSize(enc);
                    }
                    default: {
                        return 0;
                    }
                }
            }
        };

        struct Message {
            std::vector<MessageToken> msg;
            std::vector<u8> attrs;
            u32 id;

            inline size_t GetByteLength(const Encoding enc) const {
                size_t size = GetCharacterSize(enc); // All messages end with a null character
                for(const auto &token: this->msg) {
                    size += token.GetByteLength(enc);
                }
                return size;
            }
        };

        Header header;
        InfoSection info;
        DataSection data;
        std::optional<MessageIdSection> msg_id;
        std::vector<Message> messages;

        BMG() {}
        BMG(const BMG&) = delete;

        inline bool HasMessageIds() {
            return this->msg_id.has_value();
        }

        Result CreateFrom(const Encoding enc, const bool has_message_ids, const size_t attr_size, const std::vector<Message> &msgs, const u32 file_id, const ntr::fs::FileCompression comp = ntr::fs::FileCompression::None);

        Result ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        Result ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        Result WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
    };

}