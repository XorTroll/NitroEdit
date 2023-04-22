
#pragma once
#include <ui/menu/menu_Base.hpp>

namespace ui::menu {

    struct KeyboardContext {
        bool only_numeric;
        bool newline_allowed;
        u32 max_len;
    };

    void InitializeKeyboard();
    bool ShowKeyboard(const KeyboardContext &ctx, std::string &text);
    void UpdateKeyboardGraphics();

}