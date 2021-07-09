
#pragma once
#include <base_Include.hpp>

namespace gfx {

    enum class PixelFormat : u32 {
        None,
        A3I5,
        Palette4,
        Palette16,
        Palette256,
        COMP4x4,
        A5I3,
        Direct
    };

    static inline constexpr u32 GetPaletteColorCountForPixelFormat(const PixelFormat fmt) {
        switch(fmt) {
            case PixelFormat::Palette4: {
                return 4;
            }
            case PixelFormat::Palette16: {
                return 16;
            }
            case PixelFormat::Palette256: {
                return 256;
            }
            default: {
                return 0;
            }
        }
    }

    enum class MappingType : u32 {
        Mode2D,
        Mode1D32K = 16,
        Mode1D64K,
        Mode1D128K,
        Mode1D256K,
    };

    enum class CharacterFormat : u32 {
        Char,
        Bitmap,
        Max
    };

    enum class ScreenColorMode : u16 {
        Mode16x16,
        Mode256x1,
        Mode256x16
    };

    enum class ScreenFormat : u16 {
        Text,
        Affine,
        AffineExt
    };

    struct ScreenDataValue {
        union {
            struct {
                u16 tile_number : 10;
                u16 transform : 2;
                u16 plt_idx : 4;
            };
            u16 raw_val;
        };
    };
    static_assert(sizeof(ScreenDataValue) == sizeof(u16));

    namespace abgr8888 {

        struct Color {
            union {
                struct {
                    u8 r;
                    u8 g;
                    u8 b;
                    u8 a;
                } clr;
                u32 raw_val;
            };

            static constexpr u32 RedShift = 0;
            static constexpr u32 RedBits = 8;

            static constexpr u32 GreenShift = 8;
            static constexpr u32 GreenBits = 8;

            static constexpr u32 BlueShift = 16;
            static constexpr u32 BlueBits = 8;

            static constexpr u32 AlphaShift = 24;
            static constexpr u32 AlphaBits = 8;
        };
        static_assert(sizeof(Color) == sizeof(u32));

        constexpr Color TransparentColor = { .clr = { .r = 0, .g = 0, .b = 0, .a = 0 } };
        constexpr Color BlackColor = { .clr = { .r = 0, .g = 0, .b = 0, .a = 0xff } };
    
    }

    namespace xbgr1555 {

        struct Color {
            union {
                struct {
                    u16 r : 5;
                    u16 g : 5;
                    u16 b : 5;
                    u16 x : 1;
                } clr;
                u16 raw_val;
            };

            static constexpr u32 RedShift = 0;
            static constexpr u32 RedBits = 5;

            static constexpr u32 GreenShift = 5;
            static constexpr u32 GreenBits = 5;

            static constexpr u32 BlueShift = 10;
            static constexpr u32 BlueBits = 5;

            static constexpr u32 AlphaShift = 0;
            static constexpr u32 AlphaBits = 0;
        };
        static_assert(sizeof(Color) == sizeof(u16));

    }

    namespace abgr1555 {

        struct Color {
            union {
                struct {
                    u16 r : 5;
                    u16 g : 5;
                    u16 b : 5;
                    u16 a : 1;
                } clr;
                u16 raw_val;
            };

            static constexpr u32 RedShift = 0;
            static constexpr u32 RedBits = 5;

            static constexpr u32 GreenShift = 5;
            static constexpr u32 GreenBits = 5;

            static constexpr u32 BlueShift = 10;
            static constexpr u32 BlueBits = 5;

            static constexpr u32 AlphaShift = 15;
            static constexpr u32 AlphaBits = 1;
        };
        static_assert(sizeof(Color) == sizeof(u16));

    }

    template<typename C1, typename C2>
    inline constexpr C2 ConvertColor(const C1 clr) {
        if constexpr(std::is_same_v<C1, C2>) {
            return clr;
        }

        constexpr auto c1_r_mask = ~(UINT32_MAX << C1::RedBits);
        constexpr auto c2_r_mask = ~(UINT32_MAX << C2::RedBits);
        const auto r = ((((clr.raw_val >> C1::RedShift) & c1_r_mask) * 0xff) + c1_r_mask / 2) / c1_r_mask;
        const decltype(C2::raw_val) r_cmp = static_cast<decltype(C2::raw_val)>(((r * c2_r_mask + 0x7f) / 0xff) << C2::RedShift);
        
        constexpr auto c1_g_mask = ~(UINT32_MAX << C1::GreenBits);
        constexpr auto c2_g_mask = ~(UINT32_MAX << C2::GreenBits);
        const auto g = ((((clr.raw_val >> C1::GreenShift) & c1_g_mask) * 0xff) + c1_g_mask / 2) / c1_g_mask;
        const decltype(C2::raw_val) g_cmp = static_cast<decltype(C2::raw_val)>(((g * c2_g_mask + 0x7f) / 0xff) << C2::GreenShift);

        constexpr auto c1_b_mask = ~(UINT32_MAX << C1::BlueBits);
        constexpr auto c2_b_mask = ~(UINT32_MAX << C2::BlueBits);
        const auto b = ((((clr.raw_val >> C1::BlueShift) & c1_b_mask) * 0xff) + c1_b_mask / 2) / c1_b_mask;
        const decltype(C2::raw_val) b_cmp = static_cast<decltype(C2::raw_val)>(((b * c2_b_mask + 0x7f) / 0xff) << C2::BlueShift);

        if constexpr((C1::AlphaBits > 0) && (C2::AlphaBits > 0)) {
            constexpr auto c1_a_mask = ~(UINT32_MAX << C1::AlphaBits);
            constexpr auto c2_a_mask = ~(UINT32_MAX << C2::AlphaBits);
            const auto a = ((((clr.raw_val >> C1::AlphaShift) & c1_a_mask) * 0xff) + c1_a_mask / 2) / c1_a_mask;
            const decltype(C2::raw_val) a_cmp = static_cast<decltype(C2::raw_val)>(((a * c2_a_mask + 0x7f) / 0xff) << C2::AlphaShift);
            
            return {
                .raw_val = static_cast<decltype(C2::raw_val)>(r_cmp | g_cmp | b_cmp | a_cmp)
            };
        }
        else if constexpr(C1::AlphaBits <= 0) {
            constexpr auto c2_a_mask = ~(UINT32_MAX << C2::AlphaBits);
            const auto a = 0xff;
            const decltype(C2::raw_val) a_cmp = static_cast<decltype(C2::raw_val)>(((a * c2_a_mask + 0x7f) / 0xff) << C2::AlphaShift);
            
            return {
                .raw_val = static_cast<decltype(C2::raw_val)>(r_cmp | g_cmp | b_cmp | a_cmp)
            };
        }
        else if constexpr(C2::AlphaBits <= 0) {
            return {
                .raw_val = static_cast<decltype(C2::raw_val)>(r_cmp | g_cmp | b_cmp)
            };
        }
    }

    constexpr size_t TileSize = 8;

}