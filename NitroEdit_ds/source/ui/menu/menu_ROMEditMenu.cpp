#include <ui/menu/menu_ROMEditMenu.hpp>
#include <ui/menu/menu_MainMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>
#include <ui/menu/menu_ROMInfoEditMenu.hpp>
#include <ui/menu/menu_ROMTitlesEditMenu.hpp>
#include <ui/menu/menu_Keyboard.hpp>
#include <ui/menu/menu_GraphicEditor.hpp>

namespace ui::menu {

    namespace {

        ntr::gfx::abgr1555::Color *g_GfxIconGfx = nullptr;
        ntr::gfx::abgr1555::Color *g_TextIconGfx = nullptr;

        std::stack<std::shared_ptr<ntr::fmt::ROM>> g_ROMStack;
        std::stack<ntr::gfx::abgr1555::Color*> g_ROMIconGfxStack;

        std::vector<ScrollMenuEntry> g_MenuEntries;

        void OnOtherInput(const u32 k_down) {
            if(k_down & KEY_B) {
                const auto res = ShowSaveConfirmationDialog();
                switch(res) {
                    case DialogResult::Cancel: {
                        return;
                    }
                    case DialogResult::Yes: {
                        auto &cur_rom = GetCurrentROM();
                        const auto rc = SaveExternalFsFile(cur_rom, "ROM");
                        if(rc.IsFailure()) {
                            ShowOkDialog("Unable to save ROM:\n'" + FormatResult(rc) + "'");
                            // Treat this as a cancel
                            return;
                        }
                        // Fallthrough
                    }
                    case DialogResult::No: {
                        break;
                    }
                }
                g_ROMStack.pop();
                auto &cur_rom_icon_gfx = g_ROMIconGfxStack.top();
                delete[] cur_rom_icon_gfx;
                g_ROMIconGfxStack.pop();
                RestoreFileBrowseMenu();
            }
        }

    }

    void InitializeROMEditMenu() {
        u32 w;
        u32 h;
        ASSERT_SUCCESSFUL(LoadPNGGraphics("ui/gfx_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_GfxIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_SUCCESSFUL(LoadPNGGraphics("ui/text_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_TextIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        const auto fs_icon = GetFsIcon();
        g_MenuEntries = {
            {
                g_GfxIconGfx, "Edit ROM icon",
                []() {
                    auto &cur_rom = GetCurrentROM();
                    auto &cur_rom_icon_gfx = g_ROMIconGfxStack.top();
                    auto plt_copy = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(ntr::gfx::IconPaletteColorCount);
                    const auto plt = reinterpret_cast<ntr::gfx::xbgr1555::Color*>(cur_rom->banner.icon_plt);
                    // First color transparent
                    for(u32 i = 1; i < ntr::gfx::IconPaletteColorCount; i++) {
                        plt_copy[i].raw_val = ntr::gfx::ConvertColor<ntr::gfx::xbgr1555::Color, ntr::gfx::abgr1555::Color>(plt[i]).raw_val;
                    }
                    if(ShowGraphicEditor(cur_rom_icon_gfx, plt_copy, ntr::gfx::IconPaletteColorCount, ntr::gfx::IconWidth, ntr::gfx::IconHeight)) {
                        for(u32 i = 1; i < ntr::gfx::IconPaletteColorCount; i++) {
                            plt[i].raw_val = ntr::gfx::ConvertColor<ntr::gfx::abgr1555::Color, ntr::gfx::xbgr1555::Color>(plt_copy[i]).raw_val;
                        }
                        auto rom_gfx_c = ntr::util::NewArray<ntr::gfx::abgr8888::Color>(ntr::gfx::IconWidth * ntr::gfx::IconHeight);
                        for(u32 i = 0; i < ntr::gfx::IconWidth * ntr::gfx::IconHeight; i++) {
                            rom_gfx_c[i].raw_val = ntr::gfx::ConvertColor<ntr::gfx::abgr1555::Color, ntr::gfx::abgr8888::Color>(cur_rom_icon_gfx[i]).raw_val;
                        }
                        u8 *icon_char;
                        u8 *icon_plt;
                        ntr::Result rc;
                        RunWithDialog("Saving icon...", [&]() {
                            rc = ntr::gfx::ConvertBannerIconFromRgba(rom_gfx_c, icon_char, icon_plt);
                        });
                        delete[] rom_gfx_c;
                        if(rc.IsSuccess()) {
                            std::memcpy(cur_rom->banner.icon_chr, icon_char, ntr::gfx::IconCharSize);
                            std::memcpy(cur_rom->banner.icon_plt, icon_plt, ntr::gfx::IconPaletteSize);
                            delete[] icon_char;
                            delete[] icon_plt;
                            ShowOkDialog("Icon saved!");
                        }
                        else {
                            ShowOkDialog("Error saving icon:\n'" + FormatResult(rc) + "'");
                        }
                    }
                    delete[] plt_copy;
                }
            },
            {
                g_TextIconGfx, "Edit ROM info",
                LoadROMInfoEditMenu
            },
            {
                g_TextIconGfx, "Edit ROM titles",
                LoadROMTitlesEditMenu
            },
            {
                fs_icon, "Browse ROM files",
                []() {
                    auto &cur_rom = GetCurrentROM();
                    LoadNitroFsFileBrowseMenu(cur_rom, std::addressof(cur_rom->nitro_fs), FileSystemKind::ROM);
                }
            },
        };
    }

    void LoadROMEditMenu(const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle, ntr::gfx::abgr1555::Color *icon_gfx) {
        ntr::Result rc;
        auto &cur_rom = g_ROMStack.emplace(std::make_shared<ntr::fmt::ROM>());
        RunWithDialog("Loading ROM...", [&]() {
            rc = cur_rom->ReadFrom(path, file_handle);
        });

        if(rc.IsSuccess()) {
            auto cur_rom_icon_gfx = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(ntr::gfx::IconWidth * ntr::gfx::IconHeight);
            std::memcpy(cur_rom_icon_gfx, icon_gfx, ntr::gfx::IconWidth * ntr::gfx::IconHeight * sizeof(ntr::gfx::abgr1555::Color));
            g_ROMIconGfxStack.push(cur_rom_icon_gfx);
            RestoreROMEditMenu();
        }
        else {
            g_ROMStack.pop();
            ShowOkDialog("Unable to load ROM:\n'" + FormatResult(rc) + "'");
        }
    }

    void RestoreROMEditMenu() {
        return LoadScrollMenu(g_MenuEntries, OnOtherInput);
    }

    bool HasLoadedROM() {
        return !g_ROMStack.empty();
    }

    std::shared_ptr<ntr::fmt::ROM> &GetCurrentROM() {
        return g_ROMStack.top();
    }

    ntr::gfx::abgr1555::Color *GetGfxIcon() {
        return g_GfxIconGfx;
    }

    ntr::gfx::abgr1555::Color *GetTextIcon() {
        return g_TextIconGfx;
    }

}