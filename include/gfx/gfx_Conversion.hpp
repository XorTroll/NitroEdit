
#pragma once
#include <gfx/gfx_Base.hpp>

namespace gfx {

    struct GraphicsToRgbaContext {
        const u8 *gfx_data;
        size_t gfx_data_size;
        const u8 *plt_data;
        size_t plt_data_size;
        const u8 *scr_data;
        size_t scr_data_size;
        u32 scr_width;
        u32 scr_height;
        u32 plt_idx;
        u32 def_width;
        u32 def_height;
        PixelFormat pix_fmt;
        CharacterFormat char_fmt;
        bool first_color_transparent;

        abgr8888::Color *out_rgba;
        u32 out_width;
        u32 out_height;
    };

    bool ConvertGraphicsToRgba(GraphicsToRgbaContext &ctx);

    struct RgbaToGraphicsContext {
        const abgr8888::Color *rgba_data;
        u32 width;
        u32 height;
        PixelFormat pix_fmt;
        CharacterFormat char_fmt;
        bool gen_scr_data;

        u8 *out_gfx_data;
        size_t out_gfx_data_size;
        u8 *out_plt_data;
        size_t out_plt_data_size;
        u8 *out_scr_data;
        size_t out_scr_data_size;
    };

    bool ConvertRgbaToGraphics(RgbaToGraphicsContext &ctx);

}