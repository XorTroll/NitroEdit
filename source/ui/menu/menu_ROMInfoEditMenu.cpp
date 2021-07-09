#include <ui/menu/menu_ROMInfoEditMenu.hpp>
#include <ui/menu/menu_MainMenu.hpp>
#include <ui/menu/menu_ROMEditMenu.hpp>
#include <ui/menu/menu_Keyboard.hpp>

namespace ui::menu {

    namespace {

        std::vector<ScrollMenuEntry> g_MenuEntries;

        void OnOtherInput(const u32 k_down) {
            if(k_down & KEY_B) {
                RestoreROMEditMenu();
            }
        }

    }

    void InitializeROMInfoEditMenu() {
        const auto text_icon_gfx = GetTextIcon();
        g_MenuEntries = {
            {
                text_icon_gfx, "Edit game title",
                []() {
                    auto &cur_rom = GetCurrentROM();
                    auto game_title = cur_rom.header.GetGameTitle();
                    const KeyboardContext ctx = {
                        .only_numeric = false,
                        .newline_allowed = false,
                        .max_len = sizeof(cur_rom.header.game_title)
                    };
                    if(ShowKeyboard(ctx, game_title)) {
                        cur_rom.header.SetGameTitle(game_title);
                    }
                }
            },
            {
                text_icon_gfx, "Edit game code",
                []() {
                    auto &cur_rom = GetCurrentROM();
                    auto game_code = cur_rom.header.GetGameCode();
                    const KeyboardContext ctx = {
                        .only_numeric = false,
                        .newline_allowed = false,
                        .max_len = sizeof(cur_rom.header.game_code)
                    };
                    if(ShowKeyboard(ctx, game_code)) {
                        cur_rom.header.SetGameCode(game_code);
                    }
                }
            },
            {
                text_icon_gfx, "Edit maker code",
                []() {
                    auto &cur_rom = GetCurrentROM();
                    auto maker_code = cur_rom.header.GetMakerCode();
                    const KeyboardContext ctx = {
                        .only_numeric = true,
                        .newline_allowed = false,
                        .max_len = sizeof(cur_rom.header.maker_code)
                    };
                    if(ShowKeyboard(ctx, maker_code)) {
                        cur_rom.header.SetMakerCode(maker_code);
                    }
                }
            }
        };
    }

    void LoadROMInfoEditMenu() {
        LoadScrollMenu(g_MenuEntries, OnOtherInput);
    }

}