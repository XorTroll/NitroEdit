#include <ui/menu/menu_ROMTitlesEditMenu.hpp>
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

        template<ntr::util::SystemLanguage Lang>
        void EditTitleImpl() {
            auto &cur_rom = GetCurrentROM();
            auto game_title = ntr::util::ConvertFromUnicode(cur_rom.banner.GetGameTitle(Lang));
            const KeyboardContext ctx = {
                .only_numeric = false,
                .newline_allowed = true,
                .max_len = ntr::fmt::ROM::GameTitleLength
            };
            if(ShowKeyboard(ctx, game_title)) {
                cur_rom.banner.SetGameTitle(Lang, ntr::util::ConvertToUnicode(game_title));
            }
        }

    }

    void InitializeROMTitlesEditMenu() {
        const auto text_icon_gfx = GetTextIcon();
        g_MenuEntries = {
            {
                text_icon_gfx, "Edit Japanese title",
                EditTitleImpl<ntr::util::SystemLanguage::Ja>
            },
            {
                text_icon_gfx, "Edit English title",
                EditTitleImpl<ntr::util::SystemLanguage::En>
            },
            {
                text_icon_gfx, "Edit French title",
                EditTitleImpl<ntr::util::SystemLanguage::Fr>
            },
            {
                text_icon_gfx, "Edit German title",
                EditTitleImpl<ntr::util::SystemLanguage::Ge>
            },
            {
                text_icon_gfx, "Edit Italian title",
                EditTitleImpl<ntr::util::SystemLanguage::It>
            },
            {
                text_icon_gfx, "Edit Spanish title",
                EditTitleImpl<ntr::util::SystemLanguage::Es>
            },
        };
    }

    void LoadROMTitlesEditMenu() {
        LoadScrollMenu(g_MenuEntries, OnOtherInput);
    }

}