
#pragma once
#include <gfx/gfx_Conversion.hpp>

namespace gfx {

    constexpr size_t IconWidth = 32;
    constexpr size_t IconHeight = 32;

    constexpr size_t IconCharSize = 0x200;
    constexpr size_t IconPaletteSize = 0x20;
    constexpr size_t IconPaletteColorCount = IconPaletteSize / sizeof(xbgr1555::Color);

    constexpr PixelFormat IconPixelFormat = PixelFormat::Palette16;
    constexpr CharacterFormat IconCharacterFormat = CharacterFormat::Char;

    inline bool ConvertBannerIconToRgba(const u8 *icon_char, const u8 *icon_plt, abgr8888::Color *&out_rgba) {
        GraphicsToRgbaContext ctx = {
            .gfx_data = icon_char,
            .gfx_data_size = IconCharSize,
            .plt_data = icon_plt,
            .plt_data_size = IconPaletteSize,
            .plt_idx = 0,
            .def_width = IconWidth,
            .def_height = IconHeight,
            .pix_fmt = IconPixelFormat,
            .char_fmt = IconCharacterFormat,
            .first_color_transparent = true
        };
        if(!ConvertGraphicsToRgba(ctx)) {
            return false;
        }
        if((ctx.out_width != IconWidth) || (ctx.out_height != IconHeight)) {
            return false;
        }

        out_rgba = ctx.out_rgba;
        return true;
    }

    inline bool ConvertBannerIconFromRgba(const abgr8888::Color *icon, u8 *&out_icon_char, u8 *&out_icon_plt) {
        RgbaToGraphicsContext ctx = {
            .rgba_data = icon,
            .width = IconWidth,
            .height = IconHeight,
            .pix_fmt = IconPixelFormat,
            .char_fmt = IconCharacterFormat,
            .gen_scr_data = false
        };
        if(!ConvertRgbaToGraphics(ctx)) {
            return false;
        }
        if((ctx.out_gfx_data_size != IconCharSize) || (ctx.out_plt_data_size != IconPaletteSize)) {
            return false;
        }

        out_icon_char = ctx.out_gfx_data;
        out_icon_plt = ctx.out_plt_data;
        return true;
    }

}