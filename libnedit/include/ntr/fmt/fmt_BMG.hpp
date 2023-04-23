
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

        struct Header : public MagicStartBase<u64, 0x31676D624753454D /* "MESGbmg1" */ > {
            u32 file_size;
            u32 section_count;
            Encoding encoding;
            u8 unk[15];
        };

        struct InfoSection : public CommonBlock<0x31464E49 /* "INF1" */ > {
            u16 offset_count;
            u16 entry_size;
            u32 file_id;

            static constexpr size_t OffsetSize = sizeof(u32);

            inline size_t GetAttributesSize() {
                return this->entry_size - OffsetSize;
            }
        };

        struct DataSection : public CommonBlock<0x31544144 /* "DAT1" */ > {
        };

        struct String {
            std::u16string str;
            std::vector<u8> attrs;

            inline size_t GetByteLength(const Encoding enc) const {
                // null-terminator is also included
                return (str.length() + 1) * GetCharacterSize(enc);
            }
        };

        Header header;
        InfoSection info;
        DataSection data;
        std::vector<String> strings;

        BMG() {}
        BMG(const BMG&) = delete;

        bool CreateFrom(const Encoding enc, const size_t attr_size, const std::vector<String> &strs, const u32 file_id, const ntr::fs::FileCompression comp);

        bool ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        bool WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
    };

}