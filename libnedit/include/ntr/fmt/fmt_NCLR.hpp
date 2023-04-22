
#pragma once
#include <ntr/fmt/fmt_Common.hpp>
#include <ntr/gfx/gfx_Base.hpp>

namespace ntr::fmt {

    struct NCLR : public fs::FileFormat {
        
        struct Header : public CommonMultiMagicHeader<0x4E434C52 /* "RLCN" */, 0x4E435052 /* "RPCN" */> {
        };

        struct PaletteDataBlock : public CommonBlock<0x504C5454 /* "TTLP" */ > {
            gfx::PixelFormat pix_fmt;
            bool has_extended_palette;
            u8 pad[3];
            u32 data_size;
            u32 unk;
        };

        // TODO: is this block used at all?

        struct PaletteCompressDataBlock : public CommonBlock<0x50434D50 /* "PMCP" */ > {
            u16 palette_count;
            u8 pad[2];
            u32 palette_idx_table_unk;
        };

        Header header;
        PaletteDataBlock palette;
        u8 *data;

        NCLR() : data(nullptr) {}
        NCLR(const NCLR&) = delete;

        ~NCLR() {
            if(this->data != nullptr) {
                delete[] this->data;
                this->data = nullptr;
            }
        }

        bool ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        bool WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
    };

}