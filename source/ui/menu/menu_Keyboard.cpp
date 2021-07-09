#include <ui/menu/menu_Keyboard.hpp>

namespace ui::menu {

    namespace {

        struct KeyboardKey {
            u32 x;
            u32 y;
            u32 width;
            u32 height;
            bool is_pressed;
            std::function<void(const KeyboardContext&, std::string&)> on_press;
        };

        enum class KeyboardMode {
            Normal,
            Caps,
            Shift
        };

        gfx::abgr1555::Color *g_KeyboardGfx = nullptr;
        gfx::abgr1555::Color *g_KeyboardPressedGfx = nullptr;
        gfx::abgr1555::Color *g_KeyboardCapsGfx = nullptr;
        gfx::abgr1555::Color *g_KeyboardCapsPressedGfx = nullptr;
        gfx::abgr1555::Color *g_KeyboardShiftGfx = nullptr;
        gfx::abgr1555::Color *g_KeyboardShiftPressedGfx = nullptr;

        gfx::abgr1555::Color *g_KeyboardCurrentGfx = nullptr;
        gfx::abgr1555::Color *g_KeyboardCurrentPressedGfx = nullptr;
        u32 g_KeyboardWidth = 0;
        u32 g_KeyboardHeight = 0;
        u32 g_KeyboardOffset = 0;
        KeyboardMode g_KeyboardMode = KeyboardMode::Normal;

        ui::Sprite g_Keyboard = {};
        ui::Sprite g_KeyboardText = {};

        void SetButtonTexts() {
            ResetButtonTexts();
            AddButtonText("A", "Submit");
            AddButtonText("B", "Cancel");
        }

        inline bool CheckOnlyNumeric(const KeyboardContext &ctx) {
            if(ctx.only_numeric) {
                ShowOkDialog("Only numeric characters are allowed!");
                SetButtonTexts();
                return false;
            }
            return true;
        }

        inline bool CheckTextLength(const KeyboardContext &ctx, std::string &text) {
            if(ctx.max_len > 0) {
                if(text.length() >= ctx.max_len) {
                    ShowOkDialog("The text cannot be longer!");
                    SetButtonTexts();
                    while(text.length() > ctx.max_len) {
                        text.pop_back();
                    }
                    return false;
                }
            }
            return true;
        }

        inline bool CheckNewline(const KeyboardContext &ctx, const char key) {
            if(!ctx.newline_allowed) {
                if((key == '\n') || (key == '\r')) {
                    ShowOkDialog("New lines are not allowed!");
                    SetButtonTexts();
                    return false;
                }
            }
            return true;
        }

        inline void HandleNumericKeyImpl(const KeyboardContext &ctx, std::string &text, const char key) {
            if(CheckTextLength(ctx, text)) {
                text.insert(g_KeyboardOffset, 1, key);
                g_KeyboardOffset++;
            }
        }

        inline void HandleNonNumericKeyImpl(const KeyboardContext &ctx, std::string &text, const char key) {
            if(CheckTextLength(ctx, text) && CheckOnlyNumeric(ctx) && CheckNewline(ctx, key)) {
                text.insert(g_KeyboardOffset, 1, key);
                g_KeyboardOffset++;
            }
        }

        inline void HandleNumericKey(const KeyboardContext &ctx, std::string &text, const char normal_key, const char caps_shift_key) {
            switch(g_KeyboardMode) {
                case KeyboardMode::Normal: {
                    HandleNumericKeyImpl(ctx, text, normal_key);
                    break;
                }
                case KeyboardMode::Caps:
                case KeyboardMode::Shift: {
                    HandleNumericKeyImpl(ctx, text, caps_shift_key);
                    break;
                }
            }
        }
        
        inline void HandleNonNumericKey(const KeyboardContext &ctx, std::string &text, const char normal_key, const char caps_key, const char shift_key) {
            switch(g_KeyboardMode) {
                case KeyboardMode::Normal: {
                    HandleNonNumericKeyImpl(ctx, text, normal_key);
                    break;
                }
                case KeyboardMode::Caps: {
                    HandleNonNumericKeyImpl(ctx, text, caps_key);
                    break;
                }
                case KeyboardMode::Shift: {
                    HandleNonNumericKeyImpl(ctx, text, shift_key);
                    break;
                }
            }
        }

        inline void UpdateText(const std::string &text) {
            auto text_copy = text;
            text_copy.insert(g_KeyboardOffset, 1, '|');
            ASSERT_TRUE(GetTextFont().RenderText(text_copy, 12, 12, NormalLineHeight, 232, true, gfx::abgr8888::BlackColor, false, g_KeyboardText));
            ASSERT_TRUE(g_KeyboardText.Load());
        }

        void ChangeMode(const KeyboardMode new_mode) {
            if(g_KeyboardMode == new_mode) {
                g_KeyboardMode = KeyboardMode::Normal;
            }
            else {
                g_KeyboardMode = new_mode;
            }
            switch(g_KeyboardMode) {
                case KeyboardMode::Normal: {
                    g_KeyboardCurrentGfx = g_KeyboardGfx;
                    g_KeyboardCurrentPressedGfx = g_KeyboardPressedGfx;
                    break;
                }
                case KeyboardMode::Caps: {
                    g_KeyboardCurrentGfx = g_KeyboardCapsGfx;
                    g_KeyboardCurrentPressedGfx = g_KeyboardCapsPressedGfx;
                    break;
                }
                case KeyboardMode::Shift: {
                    g_KeyboardCurrentGfx = g_KeyboardShiftGfx;
                    g_KeyboardCurrentPressedGfx = g_KeyboardShiftPressedGfx;
                    break;
                }
            }
            std::memcpy(g_Keyboard.gfx_buf, g_KeyboardCurrentGfx, g_KeyboardWidth * g_KeyboardHeight * sizeof(gfx::abgr1555::Color));
            g_Keyboard.Refresh();
        }

        KeyboardKey g_KeyboardKeys[] = {
            {
                /* 1 and ! */
                .x = 28, .y = 2, .width = 14, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '1', '!');
                }
            },
            {
                /* 2 and @ */
                .x = 43, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '2', '@');
                }
            },
            {
                /* 3 and # */
                .x = 59, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '3', '#');
                }
            },
            {
                /* 4 and $ */
                .x = 75, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '4', '$');
                }
            },
            {
                /* 5 and % */
                .x = 91, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '5', '%');
                }
            },
            {
                /* 6 and ^ */
                .x = 107, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '6', '^');
                }
            },
            {
                /* 7 and & */
                .x = 123, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '7', '&');
                }
            },
            {
                /* 8 and * */
                .x = 139, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '8', '*');
                }
            },
            {
                /* 9 and ( */
                .x = 155, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '9', '(');
                }
            },
            {
                /* 0 and ) */
                .x = 171, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNumericKey(ctx, text, '0', ')');
                }
            },
            {
                /* - and _ */
                .x = 187, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, '-', '-', '_');
                }
            },
            {
                /* = and + */
                .x = 203, .y = 2, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, '=', '=', '+');
                }
            },
            {
                /* q and Q */
                .x = 36, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'q', 'Q', 'Q');
                }
            },
            {
                /* w and W */
                .x = 52, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'w', 'W', 'W');
                }
            },
            {
                /* e and E */
                .x = 68, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'e', 'E', 'E');
                }
            },
            {
                /* r and R */
                .x = 84, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'r', 'R', 'R');
                }
            },
            {
                /* t and T */
                .x = 100, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 't', 'T', 'T');
                }
            },
            {
                /* y and Y */
                .x = 116, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'y', 'Y', 'Y');
                }
            },
            {
                /* u and U */
                .x = 132, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'u', 'U', 'U');
                }
            },
            {
                /* i and I */
                .x = 148, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'i', 'I', 'I');
                }
            },
            {
                /* o and O */
                .x = 164, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'o', 'O', 'O');
                }
            },
            {
                /* p and P */
                .x = 180, .y = 17, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'p', 'P', 'P');
                }
            },
            {
                /* Delete */
                .x = 196, .y = 17, .width = 25, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    if(!text.empty() && (g_KeyboardOffset > 0)) {
                        const auto prev_len = text.length();
                        g_KeyboardOffset--;
                        text.erase(g_KeyboardOffset, 1);
                        if(g_KeyboardOffset == prev_len) {
                            g_KeyboardOffset--;
                        }
                    }
                }
            },
            {
                /* Caps */
                .x = 25, .y = 33, .width = 17, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return ChangeMode(KeyboardMode::Caps);
                }
            },
            {
                /* a and A */
                .x = 43, .y = 33, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'a', 'A', 'A');
                }
            },
            {
                /* s and S */
                .x = 59, .y = 33, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 's', 'S', 'S');
                }
            },
            {
                /* d and D */
                .x = 75, .y = 33, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'd', 'D', 'D');
                }
            },
            {
                /* f and F */
                .x = 91, .y = 33, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'f', 'F', 'F');
                }
            },
            {
                /* g and G */
                .x = 107, .y = 33, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'g', 'G', 'G');
                }
            },
            {
                /* h and H */
                .x = 123, .y = 33, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'h', 'H', 'H');
                }
            },
            {
                /* k and J */
                .x = 139, .y = 33, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'j', 'J', 'J');
                }
            },
            {
                /* k and K */
                .x = 155, .y = 33, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'k', 'K', 'K');
                }
            },
            {
                /* l and L */
                .x = 171, .y = 33, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'l', 'L', 'L');
                }
            },
            {
                /* Enter */
                .x = 187, .y = 33, .width = 34, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, '\n', '\n', '\n');
                }
            },
            {
                /* Shift */
                .x = 25, .y = 49, .width = 25, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return ChangeMode(KeyboardMode::Shift);
                }
            },
            {
                /* z and Z */
                .x = 51, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'z', 'Z', 'Z');
                }
            },
            {
                /* x and X */
                .x = 67, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'x', 'X', 'X');
                }
            },
            {
                /* c and C */
                .x = 83, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'c', 'C', 'C');
                }
            },
            {
                /* v and V */
                .x = 99, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'v', 'V', 'V');
                }
            },
            {
                /* b and B */
                .x = 115, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'b', 'B', 'B');
                }
            },
            {
                /* n and N */
                .x = 131, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'n', 'N', 'N');
                }
            },
            {
                /* m and M */
                .x = 147, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, 'm', 'M', 'M');
                }
            },
            {
                /* , and < */
                .x = 163, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, ',', ',', '<');
                }
            },
            {
                /* . and > */
                .x = 179, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, '.', '.', '>');
                }
            },
            {
                /* / and ? */
                .x = 195, .y = 49, .width = 15, .height = 15,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, '/', '/', '?');
                }
            },
            {
                /* ; and : */
                .x = 59, .y = 65, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, ';', ';', ':');
                }
            },
            {
                /* ' and ~ */
                .x = 75, .y = 65, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, '\'', '\'', '~');
                }
            },
            {
                /* Space */
                .x = 91, .y = 65, .width = 79, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, ' ', ' ', ' ');
                }
            },
            {
                /* [ and { */
                .x = 171, .y = 65, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, '[', '[', '{');
                }
            },
            {
                /* ] and } */
                .x = 187, .y = 65, .width = 15, .height = 14,/* .is_pressed = false, */
                .on_press = [](const KeyboardContext &ctx, std::string &text) {
                    return HandleNonNumericKey(ctx, text, ']', ']', '}');
                }
            },
        };
        constexpr size_t g_KeyboardKeyCount = sizeof(g_KeyboardKeys) / sizeof(KeyboardKey);

        void LoadKeyFrom(gfx::abgr1555::Color *kbd_buf, const KeyboardKey &key) {
            for(u32 y = 0; y < key.height; y++) {
                for(u32 x = 0; x < key.width; x++) {
                    const auto cur_x = x + key.x;
                    const auto cur_y = y + key.y;
                    const auto offset = cur_y * g_KeyboardWidth + cur_x;
                    g_Keyboard.SetPixel(x + key.x, y + key.y, kbd_buf[offset]);
                }
            }
            // UpdateGraphics();
        }

    }

    void InitializeKeyboard() {
        u32 w1;
        u32 h1;
        u32 w2;
        u32 h2;

        ASSERT_TRUE(LoadPNGGraphics("ui/kbd.png", ntr::nfs::CreateSelfNitroFsFileHandle(), w1, h1, g_KeyboardGfx));
        ASSERT_TRUE(LoadPNGGraphics("ui/kbd_pressed.png", ntr::nfs::CreateSelfNitroFsFileHandle(), w2, h2, g_KeyboardPressedGfx));
        ASSERT_TRUE(w1 == w2);
        ASSERT_TRUE(h1 == h2);

        ASSERT_TRUE(LoadPNGGraphics("ui/kbd_caps.png", ntr::nfs::CreateSelfNitroFsFileHandle(), w1, h1, g_KeyboardCapsGfx));
        ASSERT_TRUE(LoadPNGGraphics("ui/kbd_caps_pressed.png", ntr::nfs::CreateSelfNitroFsFileHandle(), w2, h2, g_KeyboardCapsPressedGfx));
        ASSERT_TRUE(w1 == w2);
        ASSERT_TRUE(h1 == h2);

        ASSERT_TRUE(LoadPNGGraphics("ui/kbd_shift.png", ntr::nfs::CreateSelfNitroFsFileHandle(), w1, h1, g_KeyboardShiftGfx));
        ASSERT_TRUE(LoadPNGGraphics("ui/kbd_shift_pressed.png", ntr::nfs::CreateSelfNitroFsFileHandle(), w2, h2, g_KeyboardShiftPressedGfx));
        ASSERT_TRUE(w1 == w2);
        ASSERT_TRUE(h1 == h2);

        g_KeyboardWidth = w1;
        g_KeyboardHeight = h1;
    }

    bool ShowKeyboard(const KeyboardContext &ctx, std::string &text) {
        SetScrollMenuVisible(false);
        SetButtonTexts();
        g_Keyboard.Dispose();
        g_KeyboardText.Dispose();
        auto kbd_gfx = util::NewArray<gfx::abgr1555::Color>(g_KeyboardWidth * g_KeyboardHeight);
        ASSERT_TRUE(g_Keyboard.CreateLoadFrom(true, 5, 106, kbd_gfx, g_KeyboardWidth, g_KeyboardHeight, true));
        ChangeMode(KeyboardMode::Normal);

        g_KeyboardOffset = text.length();
        if(!text.empty()) {
            UpdateText(text);
        }

        auto res = false;
        while(true) {
            scanKeys();
            const auto k_down = keysDown();
            const auto k_held = keysHeld();

            if(k_down & KEY_A) {
                res = true;
                break;
            }
            else if(k_down & KEY_B) {
                res = false;
                break;
            }
            else if(k_down & KEY_LEFT) {
                if(g_KeyboardOffset > 0) {
                    g_KeyboardOffset--;
                    UpdateText(text);
                }
            }
            else if(k_down & KEY_RIGHT) {
                if(g_KeyboardOffset < text.length()) {
                    g_KeyboardOffset++;
                    UpdateText(text);
                }
            }
            for(u32 i = 0; i < g_KeyboardKeyCount; i++) {
                auto &key = g_KeyboardKeys[i];
                if(!key.is_pressed) {
                    if(k_held & KEY_TOUCH) {
                        touchPosition pos = {};
                        touchRead(&pos);
                        const auto kx = key.x + g_Keyboard.x;
                        const auto ky = key.y + g_Keyboard.y;
                        if((pos.px > kx) && (pos.py > ky) && (pos.px < (kx + key.width)) && (pos.py < (ky + key.height))) {
                            const auto prev_text_len = text.length();
                            (key.on_press)(ctx, text);
                            if(text.length() != prev_text_len) {
                                UpdateText(text);
                            }
                            LoadKeyFrom(g_KeyboardCurrentPressedGfx, key);
                            key.is_pressed = true;
                        }
                    }
                }
                else {
                    LoadKeyFrom(g_KeyboardCurrentGfx, key);
                    key.is_pressed = false;
                }
            }

            UpdateGraphics();
        }

        g_Keyboard.Dispose();
        g_KeyboardText.Dispose();
        SetScrollMenuVisible(true);
        return res;
    }

    void UpdateKeyboardGraphics() {
        g_Keyboard.Update();
        g_KeyboardText.Update();
    }

}