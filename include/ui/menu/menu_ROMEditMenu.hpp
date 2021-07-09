
#pragma once
#include <ui/menu/menu_Base.hpp>

namespace ui::menu {

    void InitializeROMEditMenu();
    void LoadROMEditMenu(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, gfx::abgr1555::Color *icon_gfx);
    void RestoreROMEditMenu();

    bool HasLoadedROM();
    ntr::ROM &GetCurrentROM();
    gfx::abgr1555::Color *GetGfxIcon();
    gfx::abgr1555::Color *GetTextIcon();

}