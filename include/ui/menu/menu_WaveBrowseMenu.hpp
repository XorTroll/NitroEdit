
#pragma once
#include <ui/menu/menu_Base.hpp>
#include <ntr/ntr_SWAR.hpp>

namespace ui::menu {

    void LoadWaveBrowseMenu(const std::string &swar_name);
    void RestoreWaveBrowseMenu();

    ntr::SWAR &GetCurrentSWAR();

}