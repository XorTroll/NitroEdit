#include <ui/menu/menu_SDATEditMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>
#include <ui/menu/menu_WaveArchiveBrowseMenu.hpp>

namespace ui::menu {

    namespace {

        ntr::fmt::SDAT g_SDAT = {};

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
        auto ok = true;
        RunWithDialog("Loading SDAT...", [&]() {
            ok = g_SDAT.ReadFrom(path, file_handle);
        });

        if(ok) {
            soundEnable();
            RestoreSDATEditMenu();
        }
        else {
            ShowOkDialog("Unable to load SDAT...");
        }
    }

    void RestoreSDATEditMenu() {
        return LoadScrollMenu(g_MenuEntries, OnOtherInput);
    }

    ntr::fmt::SDAT &GetCurrentSDAT() {
        return g_SDAT;
    }

}