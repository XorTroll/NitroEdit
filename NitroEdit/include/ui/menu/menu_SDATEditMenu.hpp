
#pragma once
#include <ui/menu/menu_Base.hpp>
#include <ntr/fmt/fmt_SDAT.hpp>

namespace ui::menu {

    void InitializeSDATEditMenu();
    void LoadSDATEditMenu(const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle);
    void RestoreSDATEditMenu();

    ntr::fmt::SDAT &GetCurrentSDAT();

}