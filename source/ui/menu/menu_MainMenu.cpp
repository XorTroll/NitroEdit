#include <ui/menu/menu_MainMenu.hpp>
#include <ui/menu/menu_FileBrowseMenu.hpp>

#include <ui/menu/menu_Keyboard.hpp>

namespace ui::menu {

    namespace {

        ui::Sprite g_Logo = {};

        gfx::abgr1555::Color *g_FsIconGfx = nullptr;
        gfx::abgr1555::Color *g_AboutIconGfx = nullptr;

        std::vector<ScrollMenuEntry> g_MenuEntries;

        void OnOtherInput(const u32 k_down) {
        }

    }

    void InitializeMainMenu() {
        ASSERT_TRUE(g_Logo.CreateLoadFrom(false, 0, 0, "ui/logo.png", ntr::nfs::CreateSelfNitroFsFileHandle()));
        g_Logo.CenterInRegion(6, 6, 244, 161);
        
        u32 w;
        u32 h;
        ASSERT_TRUE(LoadPNGGraphics("ui/fs_icon.png", ntr::nfs::CreateSelfNitroFsFileHandle(), w, h, g_FsIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_TRUE(LoadPNGGraphics("ui/about_icon.png", ntr::nfs::CreateSelfNitroFsFileHandle(), w, h, g_AboutIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        g_MenuEntries = {
            {
                g_FsIconGfx, "Browse SD card",
                []() {
                    g_Logo.Unload();
                    LoadFatFileBrowseMenu();
                }
            },
            {
                g_AboutIconGfx, "About",
                []() {
                    ShowOkDialog("TODO:\nWrite a proper about dialog ;)");
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

    gfx::abgr1555::Color *GetFsIcon() {
        return g_FsIconGfx;
    }

}