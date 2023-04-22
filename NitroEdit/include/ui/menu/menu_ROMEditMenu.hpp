
#pragma once
#include <ui/menu/menu_Base.hpp>

namespace ui::menu {

    void InitializeROMEditMenu();
    void LoadROMEditMenu(const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle, ntr::gfx::abgr1555::Color *icon_gfx);
    void RestoreROMEditMenu();

    bool HasLoadedROM();
    ntr::fmt::ROM &GetCurrentROM();
    ntr::gfx::abgr1555::Color *GetGfxIcon();
    ntr::gfx::abgr1555::Color *GetTextIcon();

}