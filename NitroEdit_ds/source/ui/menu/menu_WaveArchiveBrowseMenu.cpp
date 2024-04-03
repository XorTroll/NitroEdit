#include <ui/menu/menu_WaveArchiveBrowseMenu.hpp>
#include <ui/menu/menu_WaveBrowseMenu.hpp>
#include <ui/menu/menu_SDATEditMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>

namespace ui::menu {

    namespace {

        std::vector<ScrollMenuEntry> g_MenuEntries;

        void OnOtherInput(const u32 k_down) {
            if(k_down & KEY_B) {
                RestoreSDATEditMenu();
            }
        }

        void OnSwarSelect(const std::string &name) {
            LoadWaveBrowseMenu(name);
        }

    }

    void LoadWaveArchiveBrowseMenu() {
        g_MenuEntries.clear();
        auto &sdat = GetCurrentSDAT();
        auto music_icon_gfx = GetMusicIcon();
        const auto swar_count = sdat->wav_arc_info_record.entry_count;
        const auto has_symb = sdat->HasSymbols();
        for(u32 i = 0; i < swar_count; i++) {
            const auto base_name = has_symb ? sdat->wav_arc_symb_record.entries[i].name : std::to_string(i);
            const auto disp_name = has_symb ? base_name : "Wave archive " + base_name;
            g_MenuEntries.push_back({
                .icon_gfx = music_icon_gfx,
                .text = disp_name,
                .on_select = std::bind(OnSwarSelect, base_name)
            });
        }
        RestoreWaveArchiveBrowseMenu();
    }

    void RestoreWaveArchiveBrowseMenu() {
        return LoadScrollMenu(g_MenuEntries, OnOtherInput);
    }

}