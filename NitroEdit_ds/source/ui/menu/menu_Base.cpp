#include <ui/menu/menu_Base.hpp>
#include <ui/menu/menu_MainMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>
#include <ui/menu/menu_ROMEditMenu.hpp>
#include <ui/menu/menu_ROMInfoEditMenu.hpp>
#include <ui/menu/menu_ROMTitlesEditMenu.hpp>
#include <ui/menu/menu_SDATEditMenu.hpp>
#include <ui/menu/menu_WaveEditMenu.hpp>
#include <ui/menu/menu_Keyboard.hpp>
#include <ui/menu/menu_GraphicEditor.hpp>

namespace ui::menu {

    namespace {

        constexpr bool ShowDialogOnSub = false;

        constexpr size_t MaxScrollMenuItemCount = 5;
        constexpr size_t ScrollMenuItemHeight = 36;
        constexpr u32 ScrollMenuItemBaseX = 6;
        constexpr u32 ScrollMenuItemBaseY = 6;
        constexpr u32 ScrollMenuItemIconXBase = ScrollMenuItemBaseX + 8;
        constexpr u32 ScrollMenuItemTextXBase = ScrollMenuItemIconXBase + 32 + 8;

        ui::TextFont g_TextFont = {};
        u32 g_ScrollMenuShowIndex = 0;
        u32 g_ScrollMenuSelectedIndex = 0;

        ui::Background g_MainBackground = {};
        ui::Background g_SubBackground = {};
        
        ui::Sprite g_DialogBoxText = {};
        ui::Sprite g_DialogBox = {};

        ui::Sprite g_ScrollMenuItemSelectBox = {};
        ui::Sprite g_ScrollMenuItemIcons[MaxScrollMenuItemCount] = {};
        ui::Sprite g_ScrollMenuItemTexts[MaxScrollMenuItemCount] = {};
        std::string g_ScrollMenuItemActualTexts[MaxScrollMenuItemCount] = {};
        ui::Sprite g_ScrollMenuBar = {};
        std::vector<ScrollMenuEntry> g_ScrollMenuEntries = {};
        std::function<void(const u32)> g_ScrollMenuOnOtherInput = nullptr;

        std::function<void(const std::vector<std::string>&)> g_ScrollMenuOnSelect = nullptr;
        std::vector<u32> g_ScrollMenuSelectedItems;
        bool g_ScrollMenuIsSelecting = false;

        std::vector<ui::Sprite> g_ButtonTexts;
        u32 g_ButtonTextOffset = 0;
        bool g_ScrollMenuButtonTextsSet = false;

        ntr::gfx::abgr1555::Color *g_MenuArrowGfx;
        u32 g_MenuArrowWidth = 0;
        u32 g_MenuArrowHeight = 0;
        std::vector<ui::Sprite> g_Menus;

        inline void ScrollMenuOnFocus() {
            if(!g_ScrollMenuEntries.empty()) {
                if(g_ScrollMenuEntries[g_ScrollMenuSelectedIndex].on_focus) {
                    (g_ScrollMenuEntries[g_ScrollMenuSelectedIndex].on_focus)();
                }   
            }
        }

        void ResetScrollMenu() {
            g_ScrollMenuSelectedIndex = 0;
            g_ScrollMenuShowIndex = 0;
            g_ScrollMenuItemSelectBox.x = ScrollMenuItemBaseX;
            g_ScrollMenuItemSelectBox.y = ScrollMenuItemBaseY;
            if(g_ScrollMenuEntries.empty()) {
                g_ScrollMenuItemSelectBox.Unload();
            }
            else {
                ASSERT_SUCCESSFUL(g_ScrollMenuItemSelectBox.EnsureLoaded());
            }

            g_ScrollMenuBar.Dispose();
            if(g_ScrollMenuEntries.size() > MaxScrollMenuItemCount) {
                const auto menu_bar_w = 12;
                const auto menu_bar_h = (size_t)((double)MaxScrollMenuItemCount * (180.0 / (double)g_ScrollMenuEntries.size()));
                auto menu_bar_gfx = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(menu_bar_w * menu_bar_h);
                const ntr::gfx::abgr8888::Color menu_bar_clr = {
                    .clr = {
                        .b = 0xff,
                        .a = 0xff
                    }
                };
                const auto menu_bar_c_clr = ntr::gfx::ConvertColor<ntr::gfx::abgr8888::Color, ntr::gfx::abgr1555::Color>(menu_bar_clr);
                for(u32 i = 0; i < menu_bar_w * menu_bar_h; i++) {
                    menu_bar_gfx[i].raw_val = menu_bar_c_clr.raw_val;
                }
                ASSERT_SUCCESSFUL(g_ScrollMenuBar.CreateLoadFrom(true, 238, 6, std::move(menu_bar_gfx), menu_bar_w, menu_bar_h, true));
            }

            ScrollMenuOnFocus();
        }

        void RedrawScrollMenu() {
            u32 base_y = ScrollMenuItemBaseY;
            for(u32 i = 0; i < MaxScrollMenuItemCount; i++) {
                g_ScrollMenuItemTexts[i].Dispose();
                g_ScrollMenuItemIcons[i].Dispose();

                const auto idx = i + g_ScrollMenuShowIndex;
                if(idx >= g_ScrollMenuEntries.size()) {
                    continue;
                }

                auto draw_clr = ntr::gfx::abgr8888::BlackColor;
                if(g_ScrollMenuIsSelecting) {
                    for(const auto &sel_item : g_ScrollMenuSelectedItems) {
                        if(sel_item == idx) {
                            draw_clr = {
                                .clr = {
                                    .r = 0xff,
                                    .a = 0xff
                                }
                            };
                            break;
                        }
                    }
                }
                ASSERT_SUCCESSFUL(g_TextFont.RenderText(g_ScrollMenuEntries[idx].text, ScrollMenuItemTextXBase, 0, NormalLineHeight, ScreenWidth, true, draw_clr, false, g_ScrollMenuItemTexts[i]));
                g_ScrollMenuItemTexts[i].CenterYInRegion(base_y, ScrollMenuItemHeight);
                ASSERT_SUCCESSFUL(g_ScrollMenuItemTexts[i].Load());

                ASSERT_TRUE(g_ScrollMenuEntries[idx].icon_gfx != nullptr);
                ASSERT_SUCCESSFUL(g_ScrollMenuItemIcons[i].CreateLoadFrom(true, ScrollMenuItemIconXBase, 0, g_ScrollMenuEntries[idx].icon_gfx, ScrollMenuIconWidth, ScrollMenuIconHeight));
                g_ScrollMenuItemIcons[i].CenterYInRegion(base_y, ScrollMenuItemHeight);
                
                base_y += ScrollMenuItemHeight;
            }
            UpdateGraphics();
        }

        void ScrollMenuMoveUp() {
            if(g_ScrollMenuSelectedIndex > 0) {
                if(g_ScrollMenuShowIndex < g_ScrollMenuSelectedIndex) {
                    g_ScrollMenuSelectedIndex--;
                    g_ScrollMenuItemSelectBox.y -= ScrollMenuItemHeight;
                    ScrollMenuOnFocus();
                }
                else if(g_ScrollMenuShowIndex == g_ScrollMenuSelectedIndex) {
                    g_ScrollMenuShowIndex--;
                    g_ScrollMenuSelectedIndex--;
                    g_ScrollMenuBar.y -= (g_ScrollMenuBar.height / MaxScrollMenuItemCount);
                    RedrawScrollMenu();
                    ScrollMenuOnFocus();
                }
            }
        }

        void ScrollMenuMoveDown() {
            if((g_ScrollMenuSelectedIndex + 1) < g_ScrollMenuEntries.size()) {
                const auto k = g_ScrollMenuShowIndex + MaxScrollMenuItemCount - 1;
                if(k > g_ScrollMenuSelectedIndex) {
                    g_ScrollMenuSelectedIndex++;
                    g_ScrollMenuItemSelectBox.y += ScrollMenuItemHeight;
                    ScrollMenuOnFocus();
                }
                else if(k == g_ScrollMenuSelectedIndex) {
                    g_ScrollMenuShowIndex++;
                    g_ScrollMenuSelectedIndex++;
                    g_ScrollMenuBar.y += (g_ScrollMenuBar.height / MaxScrollMenuItemCount);
                    RedrawScrollMenu();
                    ScrollMenuOnFocus();
                }
            }
        }

        void StartDialogImpl(const std::string &msg) {
            ASSERT_SUCCESSFUL(g_DialogBox.Load());
            ASSERT_SUCCESSFUL(g_TextFont.RenderText(msg, 0, 0, NormalLineHeight, g_DialogBox.width, ShowDialogOnSub, ntr::gfx::abgr8888::BlackColor, true, g_DialogBoxText));
            g_DialogBoxText.CenterInScreen();
            ASSERT_SUCCESSFUL(g_DialogBoxText.Load());
        }

        void EndDialogImpl() {
            g_DialogBoxText.Unload();
            g_DialogBox.Unload();
        }

        inline void SetScrollMenuButtonTexts() {
            ResetButtonTexts();
            if(!g_ScrollMenuEntries.empty()) {
                AddButtonText("A", "Select");
            }
            AddButtonText("B", "Go back");
            if(g_ScrollMenuOnSelect) {
                if(g_ScrollMenuIsSelecting) {
                    AddButtonText("X", "End selection");
                }
                else {
                    AddButtonText("X", "Start selection");
                }
            }
        }

        void UpdateButtonTexts() {
            if(!g_ButtonTexts.empty()) {
                u32 text_count = 0;
                const auto margin = 8;
                auto cur_width = 6 + margin;
                const auto max_width = 244 - margin;
                for(u32 i = 0; i < g_ButtonTexts.size(); i++) {
                    auto &text_spr = g_ButtonTexts[i];
                    if((cur_width + text_spr.width + margin) > max_width) {
                        text_spr.Unload();
                        break;
                    }
                    else {
                        text_spr.x = cur_width;
                        cur_width += text_spr.width + margin;
                        text_count++;
                        ASSERT_SUCCESSFUL(text_spr.EnsureLoaded());
                    }
                }
                while(g_ButtonTexts.size() > text_count) {
                    g_ButtonTexts.pop_back();
                }
                cur_width -= margin;
                cur_width -= margin;
                cur_width -= 6;
                auto base_x = (ScreenWidth - (244 - 2 * margin)) / 2;
                for(auto &text_spr : g_ButtonTexts) {
                    text_spr.x = base_x;
                    base_x += text_spr.width + margin;
                }
            }
        }

        DialogResult ShowYesNoCancelDialogImpl(const std::string &msg, const bool has_cancel) {
            auto out_res = DialogResult::Cancel;
            RunWithoutMainLogo([&]() {
                ResetButtonTexts();
                AddButtonText("A", "Yes");
                AddButtonText("B", "No");
                if(has_cancel) {
                    AddButtonText("X", "Cancel");
                }
                StartDialogImpl(msg);

                while(true) {
                    scanKeys();
                    const auto k_down = keysDown();
                    if(k_down & KEY_A) {
                        out_res = DialogResult::Yes;
                        break;
                    }
                    else if(k_down & KEY_B) {
                        out_res = DialogResult::No;
                        break;
                    }
                    else if((k_down & KEY_X) && has_cancel) {
                        out_res = DialogResult::Cancel;
                        break;
                    }

                    UpdateGraphics();
                }

                EndDialogImpl();
            });
            return out_res;
        }

    }

    void Initialize() {
        videoSetMode(MODE_5_2D);
        vramSetBankA(VRAM_A_MAIN_BG);
        vramSetBankB(VRAM_B_MAIN_SPRITE);
        oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
        
        videoSetModeSub(MODE_5_2D);
        vramSetBankC(VRAM_C_SUB_BG);
        vramSetBankD(VRAM_D_SUB_SPRITE);
        oamInit(&oamSub, SpriteMapping_Bmp_1D_128, false);

        g_MainBackground.Create(false);
        ASSERT_SUCCESSFUL(g_MainBackground.LoadFrom("ui/main_bg.png", CreateSelfNitroFsFileHandle()));
        g_MainBackground.Refresh();
        
        g_SubBackground.Create(true);
        ASSERT_SUCCESSFUL(g_SubBackground.LoadFrom("ui/sub_bg.png", CreateSelfNitroFsFileHandle()));
        g_SubBackground.Refresh();

        ASSERT_SUCCESSFUL(g_TextFont.LoadFrom("ui/Nintendo-DS-BIOS.ttf", CreateSelfNitroFsFileHandle()));

        ASSERT_SUCCESSFUL(LoadPNGGraphics("ui/menu_arrow.png", CreateSelfNitroFsFileHandle(), g_MenuArrowWidth, g_MenuArrowHeight, g_MenuArrowGfx));
        
        ASSERT_SUCCESSFUL(g_DialogBox.CreateFrom(ShowDialogOnSub, 0, 0, "ui/dialog_box.png", CreateSelfNitroFsFileHandle()));
        g_DialogBox.CenterInScreen();

        ASSERT_SUCCESSFUL(g_ScrollMenuItemSelectBox.CreateFrom(true, ScrollMenuItemBaseX, ScrollMenuItemBaseY, "ui/select_box.png", CreateSelfNitroFsFileHandle()));

        InitializeMainMenu();
        InitializeFileBrowseMenu();
        InitializeROMEditMenu();
        InitializeROMInfoEditMenu();
        InitializeROMTitlesEditMenu();
        InitializeSDATEditMenu();
        InitializeWaveEditMenu();
        InitializeKeyboard();
        InitializeGraphicEditor();
    }

    void MainLoop() {
        while(true) {
			scanKeys();
			const auto k_down = keysDown();
            const auto k_held = keysHeld();
			if(k_held & KEY_UP) {
				ScrollMenuMoveUp();
			}
			else if(k_held & KEY_DOWN) {
				ScrollMenuMoveDown();
			}
			else if(k_down & KEY_A) {
                if(!g_ScrollMenuEntries.empty()) {
                    if(g_ScrollMenuOnSelect && g_ScrollMenuIsSelecting) {
                        auto add_item = true;
                        for(u32 i = 0; i < g_ScrollMenuSelectedItems.size(); i++) {
                            if(g_ScrollMenuSelectedItems[i] == g_ScrollMenuSelectedIndex) {
                                g_ScrollMenuSelectedItems.erase(g_ScrollMenuSelectedItems.begin() + i);
                                add_item = false;
                                break;
                            }
                        }
                        if(add_item) {
                            g_ScrollMenuSelectedItems.push_back(g_ScrollMenuSelectedIndex);
                        }
                        RedrawScrollMenu();
                    }
                    else {
                        (g_ScrollMenuEntries[g_ScrollMenuSelectedIndex].on_select)();
                    }
                }
			}
            else if(k_down & KEY_X) {
                if(g_ScrollMenuOnSelect) {
                    if(!g_ScrollMenuIsSelecting) {
                        g_ScrollMenuIsSelecting = true;
                        g_ScrollMenuButtonTextsSet = false;
                    }
                    else {
                        g_ScrollMenuIsSelecting = false;
                        g_ScrollMenuButtonTextsSet = false;
                        if(!g_ScrollMenuSelectedItems.empty()) {
                            std::vector<std::string> selected_items;
                            for(const auto &sel_item : g_ScrollMenuSelectedItems) {
                                selected_items.push_back(g_ScrollMenuEntries[sel_item].text);
                            }
                            (g_ScrollMenuOnSelect)(selected_items);
                            g_ScrollMenuSelectedItems.clear();
                            RedrawScrollMenu();
                        }
                    }
                }
            }
            else if(g_ScrollMenuOnOtherInput) {
                g_ScrollMenuOnOtherInput(k_down);
            }

            if(!g_ScrollMenuButtonTextsSet) {
                SetScrollMenuButtonTexts();
                g_ScrollMenuButtonTextsSet = true;
            }

			UpdateGraphics();
		}
    }

    void UpdateGraphics() {
        for(const auto &menu_spr: g_Menus) {
            menu_spr.Update();
        }

        for(u32 i = 0; i < MaxScrollMenuItemCount; i++) {
			g_ScrollMenuItemIcons[i].Update();
			g_ScrollMenuItemTexts[i].Update();
		}
		g_ScrollMenuItemSelectBox.Update();
        g_ScrollMenuBar.Update();

        for(const auto &text_spr : g_ButtonTexts) {
            text_spr.Update();
        }

		g_DialogBox.Update();
		g_DialogBoxText.Update();

        UpdateMainMenuGraphics();
        UpdateFileBrowseMenuGraphics();
        UpdateKeyboardGraphics();
        UpdateGraphicEditorGraphics();

		oamUpdate(&oamMain);
		swiWaitForVBlank();
		oamUpdate(&oamSub);
		swiWaitForVBlank();
    }

    void ATTR_NORETURN ShowDialogLoop(const std::string &msg) {
        RunWithoutMainLogo([&]() {
            StartDialogImpl(msg);
            UpdateGraphics();
            while(true) {
                swiWaitForVBlank();
            }
        });
        __builtin_unreachable();
    }

    void ShowOkDialog(const std::string &msg) {
        ResetButtonTexts();
        AddButtonText("A", "Ok");
        
        RunWithoutMainLogo([&]() {
            StartDialogImpl(msg);

            while(true) {
                scanKeys();
                const auto k_down = keysDown();
                if(k_down & KEY_A) {
                    break;
                }

                UpdateGraphics();
            }

            EndDialogImpl();
        });
    }

    DialogResult ShowYesNoCancelDialog(const std::string &msg) {
        return ShowYesNoCancelDialogImpl(msg, true);
    }

    DialogResult ShowYesNoDialog(const std::string &msg) {
        return ShowYesNoCancelDialogImpl(msg, false);
    }

    void RunWithDialog(const std::string &msg, std::function<void()> fn) {
        StartDialogImpl(msg);

        UpdateGraphics();
        fn();

        EndDialogImpl();
    }

    void LoadScrollMenu(std::vector<ScrollMenuEntry> entries, std::function<void(const u32)> on_other_input) {
        DisableScrollMenuSelection();
        g_ScrollMenuEntries.clear();
        g_ScrollMenuEntries = std::move(entries);
        g_ScrollMenuOnOtherInput = on_other_input;
        ResetScrollMenu();
        RedrawScrollMenu();
    }

    void SetScrollMenuVisible(const bool visible) {
        if(visible) {
            if(!g_ScrollMenuButtonTextsSet) {
                SetScrollMenuButtonTexts();
                g_ScrollMenuButtonTextsSet = true;
            }
            for(u32 i = 0; i < MaxScrollMenuItemCount; i++) {
                if(g_ScrollMenuItemIcons[i].IsCreated()) {
                    ASSERT_SUCCESSFUL(g_ScrollMenuItemIcons[i].Load());
                }
                if(g_ScrollMenuItemTexts[i].IsCreated()) {
                    ASSERT_SUCCESSFUL(g_ScrollMenuItemTexts[i].Load());
                }
            }
            ASSERT_SUCCESSFUL(g_ScrollMenuItemSelectBox.Load());
            if(g_ScrollMenuBar.IsCreated()) {
                ASSERT_SUCCESSFUL(g_ScrollMenuBar.Load());
            }
        }
        else {
            if(g_ScrollMenuButtonTextsSet) {
                ResetButtonTexts();
                g_ScrollMenuButtonTextsSet = false;
            }
            for(u32 i = 0; i < MaxScrollMenuItemCount; i++) {
                g_ScrollMenuItemIcons[i].Unload();
                g_ScrollMenuItemTexts[i].Unload();
            }
            g_ScrollMenuItemSelectBox.Unload();
            g_ScrollMenuBar.Unload();
        }
    }

    void EnableScrollMenuSelection(std::function<void(const std::vector<std::string>&)> on_select_fn) {
        g_ScrollMenuOnSelect = on_select_fn;
        g_ScrollMenuSelectedItems.clear();
        g_ScrollMenuIsSelecting = false;
    }

    void DisableScrollMenuSelection() {
        EnableScrollMenuSelection(nullptr);
        g_ScrollMenuButtonTextsSet = false;
    }

    ui::TextFont &GetTextFont() {
        return g_TextFont;
    }

    ui::Background &GetSubBackground() {
        return g_SubBackground;
    }

    void PushMenu(const std::string &name) {
        ui::Sprite menu_spr = {};

        if(!g_Menus.empty()) {
            ui::Sprite menu_arrow = {};
            const auto &last_menu_item = g_Menus.back();
            ASSERT_SUCCESSFUL(menu_arrow.CreateLoadFrom(false, last_menu_item.x, last_menu_item.y + last_menu_item.height, g_MenuArrowGfx, g_MenuArrowWidth, g_MenuArrowHeight, false));
            
            ASSERT_SUCCESSFUL(g_TextFont.RenderText(name, menu_arrow.x + menu_arrow.width, 0, NormalLineHeight, ScreenWidth, false, ntr::gfx::abgr8888::BlackColor, false, menu_spr));
            menu_spr.CenterYInSprite(menu_arrow);

            g_Menus.push_back(std::move(menu_arrow));
        }
        else {
            ASSERT_SUCCESSFUL(g_TextFont.RenderText(name, 8, 8, NormalLineHeight, ScreenWidth, false, ntr::gfx::abgr8888::BlackColor, false, menu_spr));
        }

        ASSERT_SUCCESSFUL(menu_spr.Load());
        g_Menus.push_back(std::move(menu_spr));
    }

    void PopMenu() {
        g_Menus.pop_back();
        if(!g_Menus.empty()) {
            g_Menus.pop_back();
        }
    }

    void AddButtonText(const std::string &key, const std::string &text) {
        ui::Sprite button_text_spr = {};
        const auto key_text = "[" + key + "] " + text;
        ASSERT_SUCCESSFUL(g_TextFont.RenderText(key_text, 0, 0, NormalLineHeight, ScreenWidth, false, ntr::gfx::abgr8888::BlackColor, false, button_text_spr));
        ASSERT_SUCCESSFUL(button_text_spr.Load());
        button_text_spr.CenterYInRegion(168, 18);
        g_ButtonTexts.push_back(std::move(button_text_spr));
        g_ButtonTextOffset = 0;
        g_ScrollMenuButtonTextsSet = false;
        UpdateButtonTexts();
    }

    void ResetButtonTexts() {
        g_ButtonTexts.clear();
        g_ButtonTextOffset = 0;
    }

}