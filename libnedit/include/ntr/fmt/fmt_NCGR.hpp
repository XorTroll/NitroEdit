
#pragma once
#include <ntr/fmt/fmt_Common.hpp>
#include <ntr/gfx/gfx_Base.hpp>

namespace ntr::fmt {

    struct NCGR : public fs::FileFormat {
        
        struct Header : CommonHeader<0x4E434752 /* "RGCN" */ > {
        };

        struct CharacterDataBlock : CommonBlock<0x43484152 /* "RAHC" */ > {
            u16 tile_height;
            u16 tile_width;
            gfx::PixelFormat pix_fmt;
            gfx::MappingType map_type;
            gfx::CharacterFormat char_fmt;
            u32 data_size;
            u32 unk;
        };

        struct CharacterPositionBlock : CommonBlock<0x43504F53 /* "SOPC" */ > {
            u16 src_pos_x;
            u16 src_pos_y;
            u16 src_width;
            u16 src_height;
        };

        Header header;
        CharacterDataBlock char_data;
        CharacterPositionBlock char_pos;
        u8 *data;

        NCGR() : data(nullptr) {}
        NCGR(const NCGR&) = delete;

        ~NCGR() {
            if(this->data != nullptr) {
                delete[] this->data;
                this->data = nullptr;
            }
        }

        inline constexpr void ComputeDimensions(u32 &out_width, u32 &out_height) {
            switch(this->char_data.map_type) {
                case gfx::MappingType::Mode1D32K: {
                    out_width = 32;
                    out_height = this->char_data.data_size * ((this->char_data.pix_fmt == gfx::PixelFormat::Palette16) ? 2 : 1) / 32;
                    break;
                }
                case gfx::MappingType::Mode1D64K: {
                    out_width = 64;
                    out_height = this->char_data.data_size * ((this->char_data.pix_fmt == gfx::PixelFormat::Palette16) ? 2 : 1) / 64;
                    break;
                }
                case gfx::MappingType::Mode1D128K: {
                    out_width = 128;
                    out_height = this->char_data.data_size * ((this->char_data.pix_fmt == gfx::PixelFormat::Palette16) ? 2 : 1) / 128;
                    break;
                }
                case gfx::MappingType::Mode1D256K: {
                    out_width = 256;
                    out_height = this->char_data.data_size * ((this->char_data.pix_fmt == gfx::PixelFormat::Palette16) ? 2 : 1) / 256;
                    break;
                }
                default: {
                    out_width = this->char_data.tile_width * gfx::TileSize;
                    out_height = this->char_data.tile_height * gfx::TileSize;
                    break;
                }
            }
        }

        bool ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        bool WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
    };

}