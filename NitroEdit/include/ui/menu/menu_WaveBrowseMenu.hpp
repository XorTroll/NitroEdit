
#pragma once
#include <ui/menu/menu_Base.hpp>
#include <ntr/fmt/fmt_SWAR.hpp>

namespace ui::menu {

    void LoadWaveBrowseMenu(const std::string &swar_name);
    void RestoreWaveBrowseMenu();

    ntr::fmt::SWAR &GetCurrentSWAR();

}