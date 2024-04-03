#include <ui/menu/menu_BMGEditMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>
#include <ui/menu/menu_WaveArchiveBrowseMenu.hpp>
#include <ui/menu/menu_ROMEditMenu.hpp>
#include <ui/menu/menu_Keyboard.hpp>
#include <ntr/fmt/fmt_BMG.hpp>

namespace ui::menu {

    namespace {

        ntr::fmt::BMG g_BMG = {};

        void OnOtherInput(const u32 k_down) {
            if(k_down & KEY_B) {
                const auto res = ShowSaveConfirmationDialog();
                switch(res) {
                    case DialogResult::Cancel: {
                        return;
                    }
                    case DialogResult::Yes: {
                        SaveNormalFile(g_BMG, "BMG");
                        // Fallthrough
                    }
                    case DialogResult::No: {
                        break;
                    }
                }
                g_BMG = {};
                RestoreFileBrowseMenu();
            }
        }

        void ReloadBMGEditMenu();

        void OnStringSelect(const u32 idx) {
            const KeyboardContext ctx = {
                .only_numeric = false,
                .newline_allowed = true,
                .max_len = 0
            };
            auto edit_str = ntr::util::ConvertFromUnicode(g_BMG.messages[idx].msg_str);
            if(ShowKeyboard(ctx, edit_str)) {
                g_BMG.messages[idx].msg_str = ntr::util::ConvertToUnicode(edit_str);
                ReloadBMGEditMenu();
            }
        }

        void ReloadBMGEditMenu() {
            std::vector<ScrollMenuEntry> entries;
            auto text_icon_gfx = GetTextIcon();
            for(u32 i = 0; i < g_BMG.messages.size(); i++) {
                const auto str = ntr::util::ConvertFromUnicode(g_BMG.messages[i].msg_str);
                entries.push_back({
                    .icon_gfx = text_icon_gfx,
                    .text = "Message " + std::to_string(i),
                    .on_select = std::bind(OnStringSelect, i)
                });
            }
            LoadScrollMenu(entries, OnOtherInput);
        }

    }

    void LoadBMGEditMenu(const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle) {
        ntr::Result rc;
        RunWithDialog("Loading BMG...", [&]() {
            rc = g_BMG.ReadFrom(path, file_handle);
        });

        if(rc.IsSuccess()) {
            ReloadBMGEditMenu();
        }
        else {
            ShowOkDialog("Unable to load BMG:\n'" + FormatResult(rc) + "'");
        }
    }

}