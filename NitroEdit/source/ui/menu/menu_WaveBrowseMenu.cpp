#include <ui/menu/menu_WaveBrowseMenu.hpp>
#include <ui/menu/menu_WaveArchiveBrowseMenu.hpp>
#include <ui/menu/menu_WaveEditMenu.hpp>
#include <ui/menu/menu_SDATEditMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>

namespace ui::menu {

    namespace {

        ntr::fmt::SWAR g_SWAR = {};

        std::vector<ScrollMenuEntry> g_MenuEntries;

        void OnOtherInput(const u32 k_down) {
            if(k_down & KEY_B) {
                g_SWAR = {};
                RestoreWaveArchiveBrowseMenu();
            }
        }

        void OnWaveSelect(const u32 idx) {
            LoadWaveEditMenu(idx);
        }

    }

    void LoadWaveBrowseMenu(const std::string &swar_name) {
        g_MenuEntries.clear();
        auto &sdat = GetCurrentSDAT();
        auto music_icon_gfx = GetMusicIcon();
        const auto swar_path = "wavarc/" + swar_name;
        if(g_SWAR.ReadFrom(swar_path, std::make_shared<ntr::fmt::SDATFileHandle>(sdat))) {
            for(u32 i = 0; i < g_SWAR.samples.size(); i++) {
                g_MenuEntries.push_back({
                    .icon_gfx = music_icon_gfx,
                    .text = "Wave sample " + std::to_string(i),
                    .on_select = std::bind(OnWaveSelect, i)
                });
            }
            RestoreWaveBrowseMenu();
        }
        else {
            ShowOkDialog("Invalid wave archive...");
        }
    }

    void RestoreWaveBrowseMenu() {
        return LoadScrollMenu(g_MenuEntries, OnOtherInput);
    }

    ntr::fmt::SWAR &GetCurrentSWAR() {
        return g_SWAR;
    }

}