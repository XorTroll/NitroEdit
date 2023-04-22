
#pragma once
#include <ntr/fmt/fmt_Common.hpp>
#include <ntr/gfx/gfx_Base.hpp>

namespace ntr::fmt {

    struct NSCR : public fs::FileFormat {
        
        struct Header : public CommonHeader<0x4E534352 /* "RCSN" */ > {
        };

        struct ScreenDataBlock : public CommonBlock<0x5343524E /* "NRCS" */ > {
            u16 screen_width;
            u16 screen_height;
            gfx::ScreenColorMode screen_color_mode;
            gfx::ScreenFormat screen_format;
            u32 data_size;
        };

        Header header;
        ScreenDataBlock screen_data;
        u8 *data;

        NSCR() : data(nullptr) {}
        NSCR(const NSCR&) = delete;

        ~NSCR() {
            if(this->data != nullptr) {
                delete[] this->data;
                this->data = nullptr;
            }
        }

        bool ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        bool WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
    };

}