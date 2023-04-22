
#pragma once
#include <ntr/fmt/fmt_Common.hpp>

namespace ntr::fmt {

    struct BMG : public fs::FileFormat {
        
        struct Header : public MagicStartBase<u64, 0x31676D624753454D /* "MESGbmg1" */ > {
            u32 file_size;
            u32 section_count;
            u32 unk[4];
        };

        struct InfoSection : public CommonBlock<0x31464E49 /* "INF1" */ > {
            u16 offset_count;
            u16 unk_1;
            u32 unk_2;
        };

        struct DataSection : public CommonBlock<0x31544144 /* "DAT1" */ > {
        };

        Header header;
        InfoSection info;
        DataSection data;
        std::vector<std::u16string> strings;

        BMG() {}
        BMG(const BMG&) = delete;

        bool ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        bool WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
    };

}