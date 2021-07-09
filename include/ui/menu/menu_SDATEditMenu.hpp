
#pragma once
#include <ui/menu/menu_Base.hpp>
#include <ntr/ntr_SDAT.hpp>

namespace ui::menu {

    void InitializeSDATEditMenu();
    void LoadSDATEditMenu(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle);
    void RestoreSDATEditMenu();

    ntr::SDAT &GetCurrentSDAT();

}