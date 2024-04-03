#include <ui/menu/menu_SDATEditMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>
#include <ui/menu/menu_WaveArchiveBrowseMenu.hpp>

namespace ui::menu {

    namespace {

        std::shared_ptr<ntr::fmt::SDAT> g_SDAT = {};

        std::vector<ScrollMenuEntry> g_MenuEntries;

        void OnOtherInput(const u32 k_down) {
            if(k_down & KEY_B) {
                const auto res = ShowSaveConfirmationDialog();
                switch(res) {
                    case DialogResult::Cancel: {
                        return;
                    }
                    case DialogResult::Yes: {
                        SaveExternalFsFile(g_SDAT, "SDAT");
                        // Fallthrough
                    }
                    case DialogResult::No: {
                        break;
                    }
                }
                soundDisable();
                g_SDAT = {};
                RestoreFileBrowseMenu();
            }
        }

    }

    void InitializeSDATEditMenu() {
        g_MenuEntries = {
            {
                GetMusicIcon(), "Edit wave archives",
                []() {
                    LoadWaveArchiveBrowseMenu();
                }
            }
        };
    }

    void LoadSDATEditMenu(const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle) {
        g_SDAT = std::make_shared<ntr::fmt::SDAT>();

        ntr::Result rc;
        RunWithDialog("Loading SDAT...", [&]() {
            rc = g_SDAT->ReadFrom(path, file_handle);
        });

        if(rc.IsSuccess()) {
            soundEnable();
            RestoreSDATEditMenu();
        }
        else {
            ShowOkDialog("Unable to load SDAT:\n'" + FormatResult(rc) + "'");
        }
    }

    void RestoreSDATEditMenu() {
        return LoadScrollMenu(g_MenuEntries, OnOtherInput);
    }

    std::shared_ptr<ntr::fmt::SDAT> &GetCurrentSDAT() {
        return g_SDAT;
    }

}