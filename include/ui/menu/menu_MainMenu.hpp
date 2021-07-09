
#pragma once
#include <ui/menu/menu_Base.hpp>

namespace ui::menu {

    void InitializeMainMenu();
    void LoadMainMenu();
    void UpdateMainMenuGraphics();
    void RunWithoutMainLogo(std::function<void()> fn);

    gfx::abgr1555::Color *GetFsIcon();

}