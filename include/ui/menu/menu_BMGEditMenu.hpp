
#pragma once
#include <ui/menu/menu_Base.hpp>

namespace ui::menu {

    void InitializeBMGEditMenu();
    void LoadBMGEditMenu(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle);
    void RestoreBMGEditMenu();

}