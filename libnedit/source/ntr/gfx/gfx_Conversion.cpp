#include <ntr/gfx/gfx_Conversion.hpp>
#include <ntr/util/util_Memory.hpp>

namespace ntr::gfx {

    namespace {

        struct Palette4Value {
            union {
                struct {
                    u8 idx_1 : 2;
                    u8 idx_2 : 2;
                    u8 idx_3 : 2;
                    u8 idx_4 : 2;
                } idxs;
                u8 val;
            };
        };
        static_assert(sizeof(Palette4Value) == sizeof(u8));

        struct Palette16Value {
            union {
                struct {
                    u8 idx_1 : 4;
                    u8 idx_2 : 4;
                } idxs;
                u8 val;
            };
        };
        static_assert(sizeof(Palette16Value) == sizeof(u8));

        struct Palette256Value {
            union {
                struct {
                    u8 idx;
                } idxs;
                u8 val;
            };
        };
        static_assert(sizeof(Palette256Value) == sizeof(u8));

        inline constexpr u32 ConvertedColorAt(const xbgr1555::Color *plt_clr, const u32 plt_clr_idx, const bool first_color_transparent = false) {
            if((plt_clr_idx == 0) && first_color_transparent) {
                return abgr8888::TransparentColor.raw_val;
            }
            else {
                return ConvertColor<xbgr1555::Color, abgr8888::Color>(plt_clr[plt_clr_idx]).raw_val;
            }
        }

        inline constexpr u32 ConvertedColorCustomAlpha(const xbgr1555::Color *plt_clr, const u32 plt_clr_idx, const u8 alpha) {
            auto clr = ConvertColor<xbgr1555::Color, abgr8888::Color>(plt_clr[plt_clr_idx]);
            clr.clr.a = alpha;
            return clr.raw_val;
        }

        inline constexpr size_t ComputeNeededSize(const u32 bpp, const u32 width, const u32 height) {
            return (width * height * bpp) / CHAR_BIT;
        }

        inline constexpr bool CheckBpp(const u32 bpp, const size_t gfx_size, const u32 width, const u32 height) {
            // Ensure gfx_size is big enough to hold all the expected gfx data
            return gfx_size >= ComputeNeededSize(bpp, width, height);
        }

        inline bool *CreateColorAllocatedArray(const u32 clr_count) {
            auto array = util::NewArray<bool>(clr_count);
            // Transparent color
            array[0] = true;
            return array;
        }

        inline bool TryFindAllocatedColor(xbgr1555::Color *plt, const size_t plt_count, bool *plt_allocated, const xbgr1555::Color &clr, u8 &out_idx) {
            // Skip transparent color
            // Since, for example, black (0, 0, 0, 0xff) would be translated to just 0 as xbgr1555 since the alpha gets dropped (which would be detected as transparent), skip by starting at index 1
            for(size_t k = 1; k < plt_count; k++) {
                if(plt_allocated[k]) {
                    if(clr.raw_val == plt[k].raw_val) {
                        out_idx = static_cast<u8>(k);
                        return true;
                    }
                }
            }
            return false;
        }

        inline bool TryAllocateColor(xbgr1555::Color *plt, const size_t plt_count, bool *plt_allocated, const xbgr1555::Color &clr, u8 &out_idx) {
            // Skip transparent color (same as above)
            for(size_t k = 1; k < plt_count; k++) {
                if(!plt_allocated[k]) {
                    plt[k].raw_val = clr.raw_val;
                    out_idx = static_cast<u8>(k);
                    plt_allocated[k] = true;
                    return true;
                }
            }
            return false;
        }

        inline bool HandleColorInPalette(xbgr1555::Color *plt_clr, const u32 plt_clr_count, bool *plt_allocated, const abgr8888::Color clr, u8 &out_plt_idx) {
            if(clr.clr.a == 0) {
                out_plt_idx = 0;
                return true;
            }
            const auto c_clr = ConvertColor<abgr8888::Color, xbgr1555::Color>(clr);
            if(!TryFindAllocatedColor(plt_clr, plt_clr_count, plt_allocated, c_clr, out_plt_idx)) {
                if(!TryAllocateColor(plt_clr, plt_clr_count, plt_allocated, c_clr, out_plt_idx)) {
                    return false;
                }
            }
            return true;
        }

        void JoinBlocksImpl(u32 &cur_width, u32 &cur_height, const u32 def_width, abgr8888::Color *&out_rgba) {
            const auto block_height = cur_height;
            const auto block_count = cur_width / def_width;
            const auto block_width = def_width;

            const auto new_width = def_width;
            const auto new_height = block_count * block_height;

            auto new_rgba = util::NewArray<abgr8888::Color>(new_width * new_height);
            for(size_t i = 0; i < block_count; i++) {
                const auto dst_base_y = i * block_height;
                const auto src_base_x = i * block_width;
                for(size_t y = 0; y < block_height; y++) {
                    for(size_t x = 0; x < block_width; x++) {
                        new_rgba[x + (y + dst_base_y) * block_width].raw_val = out_rgba[(src_base_x + x) + y * cur_width].raw_val;
                    }
                }
            }

            auto old_rgba = out_rgba;
            out_rgba = new_rgba;
            delete[] old_rgba;
            cur_width = new_width;
            cur_height = new_height;
        }

        Result ConvertGraphicsToRgbaImpl(GraphicsToRgbaContext &ctx, const bool do_join_blocks) {
            const auto plt_clr = reinterpret_cast<const xbgr1555::Color*>(ctx.plt_data);
            // const auto plt_clr_count = plt_data_size / sizeof(xbgr1555::Color);

            auto cur_width = ctx.def_width;
            auto cur_height = ctx.def_height;
            ctx.out_rgba = util::NewArray<abgr8888::Color>(cur_width * cur_height);

            ScopeGuard on_fail_cleanup([&]() {
                delete[] ctx.out_rgba;
            });

            switch(ctx.pix_fmt) {
                case PixelFormat::A3I5: {
                    u32 x = 0;
                    u32 y = 0;
                    for(size_t i = 0; i < ctx.gfx_data_size; i++) {
                        const auto cur_b = ctx.gfx_data[i];
                        const auto plt_clr_idx = static_cast<u32>(cur_b & 31);
                        const auto plt_clr_alpha = static_cast<u8>(((cur_b >> 5 << 2) + (cur_b >> 5 >> 1)) * 8);

                        ctx.out_rgba[cur_width * y + x].raw_val = ConvertedColorCustomAlpha(plt_clr, plt_clr_idx, plt_clr_alpha);

                        x++;
                        if(x >= cur_width) {
                            y++;
                            x = 0;
                        }
                    }
                    break;
                }
                case PixelFormat::A5I3: {
                    u32 x = 0;
                    u32 y = 0;
                    for(size_t i = 0; i < ctx.gfx_data_size; i++) {
                        const auto cur_b = ctx.gfx_data[i];
                        const auto plt_clr_idx = static_cast<u32>(cur_b & 7);
                        const auto plt_clr_alpha = static_cast<u8>((cur_b >> 3) * 8);

                        ctx.out_rgba[cur_width * y + x].raw_val = ConvertedColorCustomAlpha(plt_clr, plt_clr_idx, plt_clr_alpha);

                        x++;
                        if(x >= cur_width) {
                            y++;
                            x = 0;
                        }
                    }
                    break;
                }
                // TODO: join pltt4/16/256 in one generic code block?
                case PixelFormat::Palette4: {
                    // 2bpp (4 pixels per byte), gfx size must be, at least, (width * height) / 4
                    constexpr u32 bpp = 2;
                    if(!CheckBpp(bpp, ctx.gfx_data_size, ctx.def_width, ctx.def_height)) {
                        NTR_R_FAIL(ResultInvalidSizeForPixelFormat);
                    }
                    switch(ctx.char_fmt) {
                        case CharacterFormat::Char: {
                            const auto block_count = ctx.def_height / TileSize;
                            const auto tiles_per_block = ctx.def_width / TileSize;
                            const auto tile_count = block_count * tiles_per_block;
                            cur_width = tile_count * TileSize;
                            cur_height = TileSize;
                            
                            u32 idx = 0;
                            for(size_t i = 0; i < block_count; i++) {
                                const auto dst_base_x_1 = i * ctx.def_width;
                                for(size_t j = 0; j < tiles_per_block; j++) {
                                    const auto dst_base_x_2 = j * TileSize;
                                    for(size_t y = 0; y < TileSize; y++) {
                                        for(size_t x = 0; x < TileSize; x += 4) {
                                            const auto dst_cur_x = dst_base_x_1 + dst_base_x_2 + x;
                                            const auto dst_cur_y = y;
                                            const auto dst_offset_1 = dst_cur_y * cur_width + dst_cur_x;
                                            const auto dst_offset_2 = dst_offset_1 + 1;
                                            const auto dst_offset_3 = dst_offset_2 + 1;
                                            const auto dst_offset_4 = dst_offset_3 + 1;

                                            const Palette4Value cur_val = {
                                                .val = ctx.gfx_data[idx]
                                            };
                                            const auto plt_clr_idx_1 = static_cast<u32>(cur_val.idxs.idx_1 + 4 * ctx.plt_idx);
                                            ctx.out_rgba[dst_offset_1].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_1, ctx.first_color_transparent);
                                            const auto plt_clr_idx_2 = static_cast<u32>(cur_val.idxs.idx_2 + 4 * ctx.plt_idx);
                                            ctx.out_rgba[dst_offset_2].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_2, ctx.first_color_transparent);
                                            const auto plt_clr_idx_3 = static_cast<u32>(cur_val.idxs.idx_3 + 4 * ctx.plt_idx);
                                            ctx.out_rgba[dst_offset_3].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_3, ctx.first_color_transparent);
                                            const auto plt_clr_idx_4 = static_cast<u32>(cur_val.idxs.idx_4 + 4 * ctx.plt_idx);
                                            ctx.out_rgba[dst_offset_4].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_4, ctx.first_color_transparent);
                                            idx++;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        default: {
                            u32 x = 0;
                            u32 y = 0;
                            for(size_t i = 0; i < ctx.gfx_data_size; i++) {

                                const Palette4Value cur_val = {
                                    .val = ctx.gfx_data[i]
                                };
                                const auto plt_clr_idx_1 = static_cast<u32>(cur_val.idxs.idx_1 + 4 * ctx.plt_idx);
                                ctx.out_rgba[y * cur_width + x].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_1, ctx.first_color_transparent);
                                x++;
                                const auto plt_clr_idx_2 = static_cast<u32>(cur_val.idxs.idx_2 + 4 * ctx.plt_idx);
                                ctx.out_rgba[y * cur_width + x].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_2, ctx.first_color_transparent);
                                x++;
                                const auto plt_clr_idx_3 = static_cast<u32>(cur_val.idxs.idx_3 + 4 * ctx.plt_idx);
                                ctx.out_rgba[y * cur_width + x].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_3, ctx.first_color_transparent);
                                x++;
                                const auto plt_clr_idx_4 = static_cast<u32>(cur_val.idxs.idx_4 + 4 * ctx.plt_idx);
                                ctx.out_rgba[y * cur_width + x].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_4, ctx.first_color_transparent);
                                x++;

                                if(x >= cur_width) {
                                    y++;
                                    x = 0;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
                case PixelFormat::Palette16: {
                    // 4bpp (2 pixels per byte), gfx size must be, at least, (width * height) / 2
                    constexpr u32 bpp = 4;
                    if(!CheckBpp(bpp, ctx.gfx_data_size, ctx.def_width, ctx.def_height)) {
                        NTR_R_FAIL(ResultInvalidSizeForPixelFormat);
                    }
                    switch(ctx.char_fmt) {
                        case CharacterFormat::Char: {
                            const auto block_count = ctx.def_height / TileSize;
                            const auto tiles_per_block = ctx.def_width / TileSize;
                            const auto tile_count = block_count * tiles_per_block;
                            cur_width = tile_count * TileSize;
                            cur_height = TileSize;
                            
                            u32 idx = 0;
                            for(size_t i = 0; i < block_count; i++) {
                                const auto dst_base_x_1 = i * ctx.def_width;
                                for(size_t j = 0; j < tiles_per_block; j++) {
                                    const auto dst_base_x_2 = j * TileSize;
                                    for(size_t y = 0; y < TileSize; y++) {
                                        for(size_t x = 0; x < TileSize; x += 2) {
                                            const auto dst_cur_x = dst_base_x_1 + dst_base_x_2 + x;
                                            const auto dst_cur_y = y;
                                            const auto dst_offset_1 = dst_cur_y * cur_width + dst_cur_x;
                                            const auto dst_offset_2 = dst_offset_1 + 1;

                                            const Palette16Value cur_val = {
                                                .val = ctx.gfx_data[idx]
                                            };
                                            const auto plt_clr_idx_1 = static_cast<u32>(cur_val.idxs.idx_1 + 16 * ctx.plt_idx);
                                            ctx.out_rgba[dst_offset_1].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_1, ctx.first_color_transparent);
                                            const auto plt_clr_idx_2 = static_cast<u32>(cur_val.idxs.idx_2 + 16 * ctx.plt_idx);
                                            ctx.out_rgba[dst_offset_2].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_2, ctx.first_color_transparent);
                                            idx++;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        default: {
                            u32 x = 0;
                            u32 y = 0;
                            for(size_t i = 0; i < ctx.gfx_data_size; i++) {
                                const Palette16Value cur_val = {
                                    .val = ctx.gfx_data[i]
                                };
                                const auto plt_clr_idx_1 = static_cast<u32>(cur_val.idxs.idx_1 + 16 * ctx.plt_idx);
                                ctx.out_rgba[y * cur_width + x].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_1, ctx.first_color_transparent);
                                x++;
                                const auto plt_clr_idx_2 = static_cast<u32>(cur_val.idxs.idx_2 + 16 * ctx.plt_idx);
                                ctx.out_rgba[y * cur_width + x + 1].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx_2, ctx.first_color_transparent);
                                x++;

                                if(x >= cur_width) {
                                    y++;
                                    x = 0;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
                case PixelFormat::Palette256: {
                    // 8bpp (a pixel per byte), gfx size must be, at least, width * height
                    constexpr u32 bpp = 8;
                    if(!CheckBpp(bpp, ctx.gfx_data_size, ctx.def_width, ctx.def_height)) {
                        NTR_R_FAIL(ResultInvalidSizeForPixelFormat);
                    }
                    switch(ctx.char_fmt) {
                        case CharacterFormat::Char: {
                            const auto block_count = ctx.def_height / TileSize;
                            const auto tiles_per_block = ctx.def_width / TileSize;
                            const auto tile_count = block_count * tiles_per_block;
                            cur_width = tile_count * TileSize;
                            cur_height = TileSize;
                            
                            u32 idx = 0;
                            for(size_t i = 0; i < block_count; i++) {
                                const auto dst_base_x_1 = i * ctx.def_width;
                                for(size_t j = 0; j < tiles_per_block; j++) {
                                    const auto dst_base_x_2 = j * TileSize;
                                    for(size_t y = 0; y < TileSize; y++) {
                                        for(size_t x = 0; x < TileSize; x++) {
                                            const auto dst_cur_x = dst_base_x_1 + dst_base_x_2 + x;
                                            const auto dst_cur_y = y;
                                            const auto dst_offset = dst_cur_y * cur_width + dst_cur_x;

                                            const Palette256Value cur_val = {
                                                .val = ctx.gfx_data[idx]
                                            };
                                            const auto plt_clr_idx = static_cast<u32>(cur_val.idxs.idx + 256 * ctx.plt_idx);
                                            ctx.out_rgba[dst_offset].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx, ctx.first_color_transparent);
                                            idx++;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        default: {
                            u32 x = 0;
                            u32 y = 0;
                            for(size_t i = 0; i < ctx.gfx_data_size; i++) {
                                const Palette256Value cur_val = {
                                    .val = ctx.gfx_data[i]
                                };
                                const auto plt_clr_idx = static_cast<u32>(cur_val.idxs.idx + 256 * ctx.plt_idx);
                                ctx.out_rgba[y * cur_width + x].raw_val = ConvertedColorAt(plt_clr, plt_clr_idx, ctx.first_color_transparent);

                                if(x >= cur_width) {
                                    y++;
                                    x = 0;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
                default: {
                    // TODO: support other formats: 4x4, direct
                    NTR_R_FAIL(ResultUnsupportedPixelFormat);
                }
            }

            if(do_join_blocks && (ctx.char_fmt == CharacterFormat::Char)) {
                JoinBlocksImpl(cur_width, cur_height, ctx.def_width, ctx.out_rgba);
            }

            on_fail_cleanup.Cancel();
            ctx.out_width = cur_width;
            ctx.out_height = cur_height;
            NTR_R_SUCCEED();
        }

        Result ConvertRgbaToGraphicsImpl(RgbaToGraphicsContext &ctx) {
            const auto plt_count = GetPaletteColorCountForPixelFormat(ctx.pix_fmt);
            switch(ctx.pix_fmt) {
                case PixelFormat::Palette4: {
                    constexpr u32 bpp = 2;
                    auto out_plt = util::NewArray<xbgr1555::Color>(plt_count);
                    auto out_plt_allocated = CreateColorAllocatedArray(plt_count);
                    ctx.out_plt_data = reinterpret_cast<u8*>(out_plt);
                    ctx.out_plt_data_size = plt_count * sizeof(xbgr1555::Color);
                    const auto gfx_size = ComputeNeededSize(bpp, ctx.width, ctx.height);
                    ctx.out_gfx_data = util::NewArray<u8>(gfx_size);
                    ctx.out_gfx_data_size = gfx_size;

                    ScopeGuard on_exit_cleanup([&]() {
                        delete[] out_plt_allocated;
                    });
                    ScopeGuard on_fail_cleanup([&]() {
                        delete[] ctx.out_plt_data;
                        delete[] ctx.out_gfx_data;
                    });

                    switch(ctx.char_fmt) {
                        case CharacterFormat::Char: {
                            const auto block_count = ctx.height / TileSize;
                            const auto tiles_per_block = ctx.width / TileSize;
                            u32 idx = 0;
                            for(size_t i = 0; i < block_count; i++) {
                                const auto src_base_y = i * TileSize;
                                for(size_t j = 0; j < tiles_per_block; j++) {
                                    const auto src_base_x = j * TileSize;
                                    for(size_t y = 0; y < TileSize; y++) {
                                        for(size_t x = 0; x < TileSize; x += 4) {
                                            const auto src_cur_x = src_base_x + x;
                                            const auto src_cur_y = src_base_y + y;

                                            const auto src_offset_1 = src_cur_y * ctx.width + src_cur_x;
                                            const auto src_offset_2 = src_offset_1 + 1;
                                            const auto src_offset_3 = src_offset_2 + 1;
                                            const auto src_offset_4 = src_offset_3 + 1;
                                            const auto clr_1 = ctx.rgba_data[src_offset_1];
                                            const auto clr_2 = ctx.rgba_data[src_offset_2];
                                            const auto clr_3 = ctx.rgba_data[src_offset_3];
                                            const auto clr_4 = ctx.rgba_data[src_offset_4];

                                            u8 plt_idx_1;
                                            if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_1, plt_idx_1)) {
                                                NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                            }
                                            u8 plt_idx_2;
                                            if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_2, plt_idx_2)) {
                                                NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                            }
                                            u8 plt_idx_3;
                                            if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_3, plt_idx_3)) {
                                                NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                            }
                                            u8 plt_idx_4;
                                            if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_4, plt_idx_4)) {
                                                NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                            }

                                            const Palette4Value cur_val = {
                                                .idxs = {
                                                    .idx_1 = plt_idx_1,
                                                    .idx_2 = plt_idx_2,
                                                    .idx_3 = plt_idx_3,
                                                    .idx_4 = plt_idx_4
                                                }
                                            };
                                            ctx.out_gfx_data[idx] = cur_val.val;
                                            idx++;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        default: {
                            for(size_t i = 0; i < gfx_size; i++) {
                                const auto src_offset_1 = i * 2;
                                const auto src_offset_2 = src_offset_1 + 1;
                                const auto src_offset_3 = src_offset_2 + 1;
                                const auto src_offset_4 = src_offset_3 + 1;
                                const auto clr_1 = ctx.rgba_data[src_offset_1];
                                const auto clr_2 = ctx.rgba_data[src_offset_2];
                                const auto clr_3 = ctx.rgba_data[src_offset_3];
                                const auto clr_4 = ctx.rgba_data[src_offset_4];

                                u8 plt_idx_1;
                                if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_1, plt_idx_1)) {
                                    NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                }
                                u8 plt_idx_2;
                                if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_2, plt_idx_2)) {
                                    NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                }
                                u8 plt_idx_3;
                                if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_3, plt_idx_3)) {
                                    NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                }
                                u8 plt_idx_4;
                                if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_4, plt_idx_4)) {
                                    NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                }

                                const Palette4Value cur_val = {
                                    .idxs = {
                                        .idx_1 = plt_idx_1,
                                        .idx_2 = plt_idx_2,
                                        .idx_3 = plt_idx_3,
                                        .idx_4 = plt_idx_4
                                    }
                                };
                                ctx.out_gfx_data[i] = cur_val.val;
                            }
                            break;
                        }
                    }
                    
                    on_fail_cleanup.Cancel();
                    break;
                }
                case PixelFormat::Palette16: {
                    constexpr u32 bpp = 4;
                    auto out_plt = util::NewArray<xbgr1555::Color>(plt_count);
                    auto out_plt_allocated = CreateColorAllocatedArray(plt_count);
                    ctx.out_plt_data = reinterpret_cast<u8*>(out_plt);
                    ctx.out_plt_data_size = plt_count * sizeof(xbgr1555::Color);
                    const auto gfx_size = ComputeNeededSize(bpp, ctx.width, ctx.height);
                    ctx.out_gfx_data = util::NewArray<u8>(gfx_size);
                    ctx.out_gfx_data_size = gfx_size;

                    ScopeGuard on_exit_cleanup([&]() {
                        delete[] out_plt_allocated;
                    });
                    ScopeGuard on_fail_cleanup([&]() {
                        delete[] ctx.out_plt_data;
                        delete[] ctx.out_gfx_data;
                    });

                    switch(ctx.char_fmt) {
                        case CharacterFormat::Char: {
                            const auto block_count = ctx.height / TileSize;
                            const auto tiles_per_block = ctx.width / TileSize;
                            u32 idx = 0;
                            for(size_t i = 0; i < block_count; i++) {
                                const auto src_base_y = i * TileSize;
                                for(size_t j = 0; j < tiles_per_block; j++) {
                                    const auto src_base_x = j * TileSize;
                                    for(size_t y = 0; y < TileSize; y++) {
                                        for(size_t x = 0; x < TileSize; x += 2) {
                                            const auto src_cur_x = src_base_x + x;
                                            const auto src_cur_y = src_base_y + y;

                                            const auto src_offset_1 = src_cur_y * ctx.width + src_cur_x;
                                            const auto src_offset_2 = src_offset_1 + 1;
                                            const auto clr_1 = ctx.rgba_data[src_offset_1];
                                            const auto clr_2 = ctx.rgba_data[src_offset_2];

                                            u8 plt_idx_1;
                                            if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_1, plt_idx_1)) {
                                                NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                            }
                                            u8 plt_idx_2;
                                            if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_2, plt_idx_2)) {
                                                NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                            }

                                            const Palette16Value cur_val = {
                                                .idxs {
                                                    .idx_1 = plt_idx_1,
                                                    .idx_2 = plt_idx_2
                                                }
                                            };
                                            ctx.out_gfx_data[idx] = cur_val.val;
                                            idx++;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        default: {
                            for(size_t i = 0; i < gfx_size; i++) {
                                const auto src_offset_1 = i * 2;
                                const auto src_offset_2 = src_offset_1 + 1;
                                const auto clr_1 = ctx.rgba_data[src_offset_1];
                                const auto clr_2 = ctx.rgba_data[src_offset_2];

                                u8 plt_idx_1;
                                if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_1, plt_idx_1)) {
                                    NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                }
                                u8 plt_idx_2;
                                if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr_2, plt_idx_2)) {
                                    NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                }
                                
                                const Palette16Value cur_val = {
                                    .idxs {
                                        .idx_1 = plt_idx_1,
                                        .idx_2 = plt_idx_2
                                    }
                                };
                                ctx.out_gfx_data[i] = cur_val.val;
                            }
                            break;
                        }
                    }
                    
                    on_fail_cleanup.Cancel();
                    break;
                }
                case PixelFormat::Palette256: {
                    constexpr u32 bpp = 8;
                    auto out_plt = util::NewArray<xbgr1555::Color>(plt_count);
                    auto out_plt_allocated = CreateColorAllocatedArray(plt_count);
                    ctx.out_plt_data = reinterpret_cast<u8*>(out_plt);
                    ctx.out_plt_data_size = plt_count * sizeof(xbgr1555::Color);
                    const auto gfx_size = ComputeNeededSize(bpp, ctx.width, ctx.height);
                    ctx.out_gfx_data = util::NewArray<u8>(gfx_size);
                    ctx.out_gfx_data_size = gfx_size;

                    ScopeGuard on_exit_cleanup([&]() {
                        delete[] out_plt_allocated;
                    });
                    ScopeGuard on_fail_cleanup([&]() {
                        delete[] ctx.out_plt_data;
                        delete[] ctx.out_gfx_data;
                    });

                    switch(ctx.char_fmt) {
                        case CharacterFormat::Char: {
                            const auto block_count = ctx.height / TileSize;
                            const auto tiles_per_block = ctx.width / TileSize;
                            u32 idx = 0;
                            for(size_t i = 0; i < block_count; i++) {
                                const auto src_base_y = i * TileSize;
                                for(size_t j = 0; j < tiles_per_block; j++) {
                                    const auto src_base_x = j * TileSize;
                                    for(size_t y = 0; y < TileSize; y++) {
                                        for(size_t x = 0; x < TileSize; x++) {
                                            const auto src_cur_x = src_base_x + x;
                                            const auto src_cur_y = src_base_y + y;

                                            const auto src_offset = src_cur_y * ctx.width + src_cur_x;
                                            const auto clr = ctx.rgba_data[src_offset];

                                            u8 plt_idx;
                                            if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr, plt_idx)) {
                                                NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                            }

                                            const Palette256Value cur_val = {
                                                .idxs {
                                                    .idx = plt_idx
                                                }
                                            };
                                            ctx.out_gfx_data[idx] = cur_val.val;
                                            idx++;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        default: {
                            for(size_t i = 0; i < gfx_size; i++) {
                                const auto clr = ctx.rgba_data[i];

                                u8 plt_idx;
                                if(!HandleColorInPalette(out_plt, plt_count, out_plt_allocated, clr, plt_idx)) {
                                    NTR_R_FAIL(ResultColorPaletteAllocationFailure);
                                }
                                
                                const Palette256Value cur_val = {
                                    .idxs {
                                        .idx = plt_idx
                                    }
                                };
                                ctx.out_gfx_data[i] = cur_val.val;
                            }
                            break;
                        }
                    }

                    on_fail_cleanup.Cancel();
                    break;
                }
                default: {
                    // TODO: support other formats: a3i5, a5i3, 4x4, direct
                    NTR_R_FAIL(ResultUnsupportedPixelFormat);
                }
            }

            NTR_R_SUCCEED();
        }

        Result ConvertGraphicsToRgbaWithScreenDataImpl(GraphicsToRgbaContext &ctx) {
            if((ctx.pix_fmt != PixelFormat::Palette16) && (ctx.pix_fmt != PixelFormat::Palette256)) {
                NTR_R_FAIL(ResultInvalidPixelFormat);
            }
            if(ctx.char_fmt != CharacterFormat::Char) {
                NTR_R_FAIL(ResultInvalidCharacterFormat);
            }

            auto scr_data_vals = reinterpret_cast<const ScreenDataValue*>(ctx.scr_data);
            const auto scr_data_val_count = ctx.scr_data_size / sizeof(ScreenDataValue);
            
            u32 tmp_width = scr_data_val_count * TileSize;
            u32 tmp_height = TileSize;
            ctx.out_rgba = util::NewArray<abgr8888::Color>(tmp_width * tmp_height);

            ScopeGuard on_fail_cleanup([&]() {
                delete[] ctx.out_rgba;
            });

            const auto colors_per_plt = GetPaletteColorCountForPixelFormat(ctx.pix_fmt);
            const auto single_plt_size = colors_per_plt * sizeof(xbgr1555::Color);
            const auto plt_count = ctx.plt_data_size / single_plt_size;
            auto tmp_conv_array = util::NewArray<GraphicsToRgbaContext>(plt_count);
            for(size_t i = 0; i < plt_count; i++) {
                auto ctx_copy = ctx;
                ctx_copy.plt_idx = i;
                ConvertGraphicsToRgbaImpl(ctx_copy, false);
                tmp_conv_array[i] = ctx_copy;
            }

            ScopeGuard on_exit_cleanup([&]() {
                for(size_t i = 0; i < plt_count; i++) {
                    if(tmp_conv_array[i].out_rgba != nullptr) {
                        delete[] tmp_conv_array[i].out_rgba;
                    }
                }

                delete[] tmp_conv_array;
            });

            for(size_t i = 0; i < scr_data_val_count; i++) {
                const auto cur_val = scr_data_vals[i];
                if(cur_val.tile_number != 0) {
                    const auto tile_idx = cur_val.tile_number - 1;

                    const auto src_tile_x = tile_idx;
                    const auto src_tile_y = 0;
                    const auto src_base_x = src_tile_x * TileSize;
                    const auto src_base_y = src_tile_y * TileSize;

                    const auto dst_tile_x = i;
                    const auto dst_tile_y = 0;
                    const auto dst_base_x = dst_tile_x * TileSize;
                    const auto dst_base_y = dst_tile_y * TileSize;
                    for(size_t y = 0; y < TileSize; y++) {
                        for(size_t x = 0; x < TileSize; x++) {
                            const auto cur_src_x = x + src_base_x;
                            const auto cur_src_y = y + src_base_y;
                            const auto cur_dst_x = x + dst_base_x;
                            const auto cur_dst_y = y + dst_base_y;
                            const auto tmp_rgba_width = tmp_conv_array[cur_val.plt_idx].out_width;
                            const auto tmp_rgba_height = tmp_conv_array[cur_val.plt_idx].out_height;
                            auto tmp_rgba = tmp_conv_array[cur_val.plt_idx].out_rgba;
                            if(tmp_rgba == nullptr) {
                                NTR_R_FAIL(ResultInvalidScreenData);
                            }
                            const auto src_offset = cur_src_x + tmp_rgba_width * cur_src_y;
                            const auto dst_offset = cur_dst_x + tmp_width * cur_dst_y;
                            if(src_offset >= (tmp_rgba_width * tmp_rgba_height)) {
                                NTR_R_FAIL(ResultInvalidScreenData);
                            }
                            if(dst_offset >= (tmp_width * tmp_height)) {
                                NTR_R_FAIL(ResultInvalidScreenData);
                            }
                            ctx.out_rgba[dst_offset].raw_val = tmp_rgba[src_offset].raw_val;
                        }
                    }
                }
            }

            on_fail_cleanup.Cancel();

            JoinBlocksImpl(tmp_width, tmp_height, ctx.scr_width, ctx.out_rgba);

            ctx.out_width = tmp_width;
            ctx.out_height = tmp_height;
            NTR_R_SUCCEED();
        }

    }

    Result ConvertGraphicsToRgba(GraphicsToRgbaContext &ctx) {
        if(ctx.scr_data != nullptr) {
            return ConvertGraphicsToRgbaWithScreenDataImpl(ctx);
        }
        else {
            return ConvertGraphicsToRgbaImpl(ctx, true);
        }
    }

    Result ConvertRgbaToGraphics(RgbaToGraphicsContext &ctx) {
        if(ctx.gen_scr_data) {
            // TODO: support this
            NTR_R_FAIL(ResultScreenDataGenerationUnimplemented);
        }
        else {
            return ConvertRgbaToGraphicsImpl(ctx);
        }
    }

}