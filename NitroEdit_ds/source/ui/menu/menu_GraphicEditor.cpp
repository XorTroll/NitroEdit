#include <ui/menu/menu_GraphicEditor.hpp>

namespace ui::menu {

    namespace {

        constexpr u32 BaseSpriteSize = std::max(ScreenWidth, ScreenHeight);

        ntr::gfx::abgr1555::Color *g_GfxOrigBuf = nullptr;
        ntr::gfx::abgr1555::Color *g_GfxBuf = nullptr;
        u32 g_GfxWidth = 0;
        u32 g_GfxHeight = 0;
        ntr::gfx::abgr1555::Color *g_GfxColorPlt = nullptr;
        size_t g_GfxColorPltCount = 0;

        ntr::gfx::abgr1555::Color *g_GfxBgBuf = nullptr;
        u32 g_GfxBgWidth = 0;
        u32 g_GfxBgHeight = 0;
        ui::Sprite g_Gfx = {};
        u32 g_GfxBaseX = 0;
        u32 g_GfxBaseY = 0;
        u32 g_ZoomValue = 1;

        constexpr u32 MaxPaletteItemCount = 10;
        ui::Sprite g_PaletteItems[MaxPaletteItemCount] = {};
        ui::Sprite g_PaletteSelectBox = {};
        u32 g_PaletteMenuSelectedIndex = 0;
        u32 g_PaletteMenuShowIndex = 0;

        void DrawPixel(const u32 x, const u32 y, const ntr::gfx::abgr1555::Color clr) {
            const auto paint_clr = (clr.clr.a == 0) ? g_GfxBgBuf[(y + g_GfxBaseY) * g_GfxBgWidth + (x + g_GfxBaseX)] : clr;
            g_Gfx.SetPixel(x + g_GfxBaseX, y + g_GfxBaseY, paint_clr);
        }

        void RedrawGraphics() {
            for(u32 y = 0; y < g_GfxBgHeight; y++) {
                for(u32 x = 0; x < g_GfxBgWidth; x++) {
                    g_Gfx.SetPixel(x, y, g_GfxBgBuf[y * g_GfxBgWidth + x]);
                }
            }
            const auto width = std::min(g_GfxWidth * g_ZoomValue, BaseSpriteSize);
            const auto height = std::min(g_GfxHeight * g_ZoomValue, BaseSpriteSize);
            g_GfxBaseX = (width < BaseSpriteSize) ? ((BaseSpriteSize - width) / 2) : 0;
            g_GfxBaseY = (height < BaseSpriteSize) ? ((BaseSpriteSize - height) / 2) : 0;
            for(u32 y = 0; y < height; y += g_ZoomValue) {
                for(u32 zy = 0; zy < g_ZoomValue; zy++) {
                    for(u32 x = 0; x < width; x += g_ZoomValue) {
                        for(u32 zx = 0; zx < g_ZoomValue; zx++) {
                            const auto clr = g_GfxBuf[(y / g_ZoomValue) * g_GfxWidth + (x / g_ZoomValue)];
                            if(clr.clr.a > 0) {
                                DrawPixel(x + zx, y + zy, clr);
                            }
                        }
                    }
                }
            }
        }

        void ResetPaletteMenu() {
            g_PaletteMenuShowIndex = 0;
            g_PaletteMenuSelectedIndex = 0;
            g_PaletteSelectBox.x = 12 - 2;
            g_PaletteSelectBox.y = 140 - 2;
            ASSERT_SUCCESSFUL(g_PaletteSelectBox.EnsureLoaded());
        }

        void RedrawPaletteMenu() {
            u32 base_x = 12;
            for(u32 i = 0; i < MaxPaletteItemCount; i++) {
                g_PaletteItems[i].Dispose();

                const auto idx = i + g_PaletteMenuShowIndex;
                if(idx >= g_GfxColorPltCount) {
                    continue;
                }

                const auto cur_clr = g_GfxColorPlt[idx];
                auto plt_gfx = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(16 * 16);
                for(u32 y = 0; y < 16; y++) {
                    for(u32 x = 0; x < 16; x++) {
                        if(cur_clr.clr.a == 0) {
                            plt_gfx[y * 16 + x].raw_val = g_GfxBgBuf[y * g_GfxBgWidth + x].raw_val;
                        }
                        else {
                            plt_gfx[y * 16 + x].raw_val = cur_clr.raw_val;
                        }
                    }
                }

                ASSERT_SUCCESSFUL(g_PaletteItems[i].CreateLoadFrom(false, base_x, 140, plt_gfx, 16, 16, true));
                base_x += 16 + 8;
            }
            UpdateGraphics();
        }

        void PaletteMenuMoveLeft() {
            if(g_PaletteMenuSelectedIndex > 0) {
                if(g_PaletteMenuShowIndex < g_PaletteMenuSelectedIndex) {
                    g_PaletteMenuSelectedIndex--;
                    g_PaletteSelectBox.x -= 16 + 8;
                }
                else if(g_PaletteMenuShowIndex == g_PaletteMenuSelectedIndex) {
                    g_PaletteMenuShowIndex--;
                    g_PaletteMenuSelectedIndex--;
                    RedrawPaletteMenu();
                }
            }
        }

        void PaletteMenuMoveRight() {
            if((g_PaletteMenuSelectedIndex + 1) < g_GfxColorPltCount) {
                const auto k = g_PaletteMenuShowIndex + MaxPaletteItemCount - 1;
                if(k > g_PaletteMenuSelectedIndex) {
                    g_PaletteMenuSelectedIndex++;
                    g_PaletteSelectBox.x += 16 + 8;
                }
                else if(k == g_PaletteMenuSelectedIndex) {
                    g_PaletteMenuShowIndex++;
                    g_PaletteMenuSelectedIndex++;
                    RedrawPaletteMenu();
                }
            }
        }

        void SetButtonTexts() {
            ResetButtonTexts();
            AddButtonText("B", "Exit");
            AddButtonText("L/R", "Color");
            AddButtonText("X/Y", "Zoom");
        }

    }

    void InitializeGraphicEditor() {
        ASSERT_SUCCESSFUL(LoadPNGGraphics("ui/gfx_editor_bg.png", CreateSelfNitroFsFileHandle(), g_GfxBgWidth, g_GfxBgHeight, g_GfxBgBuf));

        ASSERT_SUCCESSFUL(g_PaletteSelectBox.CreateFrom(false, 0, 0, "ui/plt_select_box.png", CreateSelfNitroFsFileHandle()));
    }

    bool ShowGraphicEditor(ntr::gfx::abgr1555::Color *gfx_buf, ntr::gfx::abgr1555::Color *color_plt, const size_t color_plt_count, const u32 width, const u32 height) {
        SetScrollMenuVisible(false);
        SetButtonTexts();
        g_GfxOrigBuf = gfx_buf;
        if((width > BaseSpriteSize) || (height > BaseSpriteSize)) {
            ShowOkDialog("Too big texture...");
            return false;
        }
        g_GfxBuf = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(BaseSpriteSize * BaseSpriteSize);
        std::memcpy(g_GfxBuf, g_GfxOrigBuf, width * height * sizeof(ntr::gfx::abgr1555::Color));
        g_GfxWidth = width;
        g_GfxHeight = height;
        ASSERT_SUCCESSFUL(g_Gfx.CreateLoadFrom(true, 0, 0, g_GfxBgBuf, BaseSpriteSize, BaseSpriteSize, false));
        g_GfxColorPlt = color_plt;
        g_GfxColorPltCount = color_plt_count;
        g_ZoomValue = 1;

        ResetPaletteMenu();
        RedrawPaletteMenu();
        RedrawGraphics();

        auto ret = false;
        while(true) {
            scanKeys();
            const auto k_down = keysDown();
            const auto k_held = keysHeld();
            if(k_held & KEY_DOWN) {
                if((g_Gfx.y + g_GfxBgHeight) > ScreenHeight) {
                    g_Gfx.y -= g_ZoomValue;
                }
            }
            else if(k_held & KEY_UP) {
                if(g_Gfx.y < 0) {
                    g_Gfx.y += g_ZoomValue;
                }
            }
            if(k_held & KEY_LEFT) {
                if(g_Gfx.x > 0) {
                    g_Gfx.x -= g_ZoomValue;
                }
            }
            else if(k_held & KEY_RIGHT) {
                if((g_Gfx.x + BaseSpriteSize) < g_GfxBgWidth) {
                    g_Gfx.x += g_ZoomValue;
                }
            }
            if(k_held & KEY_TOUCH) {
                touchPosition pos = {};
                touchRead(&pos);

                const auto width = std::min(g_GfxWidth * g_ZoomValue, BaseSpriteSize);
                const auto height = std::min(g_GfxHeight * g_ZoomValue, BaseSpriteSize);
                const ssize_t x_diff = g_GfxBaseX + g_Gfx.x;
                const ssize_t y_diff = g_GfxBaseY + g_Gfx.y;
                if((pos.px > x_diff) && (pos.py > y_diff) && (pos.px < (x_diff + width)) && (pos.py < (y_diff + height))) {
                    const auto paint_clr = g_GfxColorPlt[g_PaletteMenuSelectedIndex];
                    const auto x = pos.px - x_diff;
                    const auto y = pos.py - y_diff;
                    const auto real_x = x / g_ZoomValue;
                    const auto real_y = y / g_ZoomValue;
                    const auto draw_z_x = real_x * g_ZoomValue;
                    const auto draw_z_y = real_y * g_ZoomValue;
                    for(u32 zy = 0; zy < g_ZoomValue; zy++) {
                        for(u32 zx = 0; zx < g_ZoomValue; zx++) {
                            DrawPixel(draw_z_x + zx, draw_z_y + zy, paint_clr);
                        }
                    }
                    g_GfxBuf[real_y * g_GfxWidth + real_x].raw_val = paint_clr.raw_val;
                }
            }
            if(k_down & KEY_X) {
                g_ZoomValue++;
                if(((g_GfxWidth * g_ZoomValue) > BaseSpriteSize) || ((g_GfxHeight * g_ZoomValue) > BaseSpriteSize)) {
                    g_ZoomValue--;
                }
                else {
                    RedrawGraphics();
                }
            }
            else if(k_down & KEY_Y) {
                if(g_ZoomValue > 1) {
                    g_ZoomValue--;
                    RedrawGraphics();
                }
            }
            if(k_down & KEY_L) {
                PaletteMenuMoveLeft();
            }
            else if(k_down & KEY_R) {
                PaletteMenuMoveRight();
            }
            if(k_down & KEY_B) {
                const auto res = ShowSaveConfirmationDialog();
                SetButtonTexts();
                if(res == DialogResult::Yes) {
                    std::memcpy(g_GfxOrigBuf, g_GfxBuf, width * height * sizeof(ntr::gfx::abgr1555::Color));
                    ret = true;
                    break;
                }
                else if(res == DialogResult::No) {
                    ret = false;
                    break;
                }
            }

            UpdateGraphics();
        }

        g_Gfx.Dispose();

        for(u32 i = 0; i < MaxPaletteItemCount; i++) {
            g_PaletteItems[i].Dispose();
        }
        g_PaletteSelectBox.Unload();

        delete[] g_GfxBuf;
        SetScrollMenuVisible(true);
        return ret;
    }

    void UpdateGraphicEditorGraphics() {
        g_Gfx.Update();

        for(u32 i = 0; i < MaxPaletteItemCount; i++) {
            g_PaletteItems[i].Update();
        }
        g_PaletteSelectBox.Update();
    }

}