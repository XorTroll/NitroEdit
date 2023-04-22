#include <ui/menu/menu_MainMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>
#include <ui/menu/menu_Keyboard.hpp>

namespace ui::menu {

    namespace {

        ui::Sprite g_Logo = {};

        ntr::gfx::abgr1555::Color *g_FsIconGfx = nullptr;
        ntr::gfx::abgr1555::Color *g_AboutIconGfx = nullptr;

        std::vector<ScrollMenuEntry> g_MenuEntries;

        void OnOtherInput(const u32 k_down) {
        }

    }

    void InitializeMainMenu() {
        ASSERT_TRUE(g_Logo.CreateLoadFrom(false, 0, 0, "ui/logo.png", CreateSelfNitroFsFileHandle()));
        g_Logo.CenterInRegion(6, 6, 244, 161);
        
        u32 w;
        u32 h;
        ASSERT_TRUE(LoadPNGGraphics("ui/fs_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_FsIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_TRUE(LoadPNGGraphics("ui/about_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_AboutIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        g_MenuEntries = {
            {
                g_FsIconGfx, "Browse filesystem",
                []() {
                    g_Logo.Unload();
                    LoadStdioFileBrowseMenu();
                }
            },
            {
                g_AboutIconGfx, "About",
                []() {
                    ShowOkDialog("NitroEdit v" NEDIT_VERSION "\n" "Nintendo DS(i) ROM editor" "\n\n" "Check out GitHub" "\n" "for more information!");
                }
            }
        };
    }

    void LoadMainMenu() {
        ASSERT_TRUE(g_Logo.EnsureLoaded());
		LoadScrollMenu(g_MenuEntries, OnOtherInput);
	}

    void UpdateMainMenuGraphics() {
        g_Logo.Update();
    }

    void RunWithoutMainLogo(std::function<void()> fn) {
        if(g_Logo.IsLoaded()) {
            g_Logo.Unload();
            fn();
            ASSERT_TRUE(g_Logo.EnsureLoaded());
        }
        else {
            fn();
        }
    }

    ntr::gfx::abgr1555::Color *GetFsIcon() {
        return g_FsIconGfx;
    }

}