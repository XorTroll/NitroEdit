#include <ui/menu/menu_FileBrowseMenu.hpp>
#include <ui/menu/menu_MainMenu.hpp>
#include <ui/menu/menu_ROMEditMenu.hpp>
#include <ui/menu/menu_Keyboard.hpp>
#include <ui/menu/menu_GraphicEditor.hpp>
#include <ui/menu/menu_SDATEditMenu.hpp>
#include <ui/menu/menu_BMGEditMenu.hpp>
#include <ntr/gfx/gfx_BannerIcon.hpp>
#include <ntr/fmt/fmt_NARC.hpp>
#include <ntr/fmt/fmt_NCGR.hpp>
#include <ntr/fmt/fmt_NCLR.hpp>
#include <ntr/fmt/fmt_NSCR.hpp>
#include <base_System.hpp>

namespace ui::menu {

    namespace {

        ntr::gfx::abgr1555::Color *g_DirectoryIconGfx = nullptr;
        ntr::gfx::abgr1555::Color *g_FileIconGfx = nullptr;
        ntr::gfx::abgr1555::Color *g_ROMIconGfx = nullptr;
        ntr::gfx::abgr1555::Color *g_MusicIconGfx = nullptr;
        ntr::gfx::abgr1555::Color *g_TexIconGfx = nullptr;
        ntr::gfx::abgr1555::Color *g_CurrentROMIconGfx = nullptr;

        ui::Sprite g_CurrentROMIcon = {};
        ui::Sprite g_CurrentROMText = {};

        struct NitroFsBrowseEntry {
            ntr::fmt::nfs::NitroFsFileFormat *file_ref_ptr;
            ntr::fmt::nfs::NitroDirectory *dir_ref_ptr;
            bool is_loaded_narc;
            std::string path;
        };

        std::stack<NitroFsBrowseEntry> g_NitroFsEntryStack;
        std::stack<ntr::fmt::NARC> g_LoadedNARCs;

        std::string g_CurrentDirectory;

        inline std::shared_ptr<ntr::fs::FileHandle> CreateCurrentNitroFsEntryFileHandle() {
            if(g_NitroFsEntryStack.top().is_loaded_narc) {
                auto &narc = *reinterpret_cast<ntr::fmt::NARC*>(g_NitroFsEntryStack.top().file_ref_ptr);
                return std::move(std::make_shared<ntr::fmt::NARCFileHandle>(narc));
            }
            else {
                auto &rom = *reinterpret_cast<ntr::fmt::ROM*>(g_NitroFsEntryStack.top().file_ref_ptr);
                return std::move(std::make_shared<ntr::fmt::ROMFileHandle>(rom));
            }
        }

        inline std::shared_ptr<ntr::fs::FileHandle> CreateCurrentFileHandleImpl(const bool is_fat) {
            if(is_fat) {
                return std::move(std::make_shared<ntr::fs::StdioFileHandle>());
            }
            else {
                return std::move(CreateCurrentNitroFsEntryFileHandle());
            }
        }

        void CleanTextIcon(const bool delete_gfx) {
            g_CurrentROMIcon.Dispose();
            g_CurrentROMText.Dispose();
            if(delete_gfx) {
                if(g_CurrentROMIconGfx != nullptr) {
                    delete[] g_CurrentROMIconGfx;
                    g_CurrentROMIconGfx = nullptr;
                }
            }
        }

        void OnStdioDirectorySelect(const std::string &dir_path) {
            LoadStdioFileBrowseMenu(ntr::fs::GetAbsolutePath(dir_path));
        }

        void OnNitroFsDirectorySelect(ntr::fmt::nfs::NitroFsFileFormat *file, ntr::fmt::nfs::NitroDirectory *dir) {
            LoadNitroFsFileBrowseMenu(file, dir, false);
        }

        void OnFileSelect(const bool is_fat, const std::string &file_path) {
            // TODO: what to do with regular files?
        }

        void OnROMSelect(const bool is_fat, const std::string &rom_path) {
            g_CurrentROMIcon.Unload();
            g_CurrentROMText.Unload();
            const auto res = ShowOpenConfirmationDialog("NDS(i) ROM");
            if(res == DialogResult::Yes) {
                LoadROMEditMenu(rom_path, CreateCurrentFileHandleImpl(is_fat), g_CurrentROMIconGfx);
                CleanTextIcon(true);
            }
            else {
                ASSERT_TRUE(g_CurrentROMIcon.EnsureLoaded());
                ASSERT_TRUE(g_CurrentROMText.EnsureLoaded());
            }
        }

        void OnNARCSelect(const bool is_fat, const std::string &narc_path) {
            const auto res = ShowOpenConfirmationDialog("archive");
            if(res == DialogResult::Yes) {
                auto &st_narc = g_LoadedNARCs.emplace();
                if(st_narc.ReadFrom(narc_path, CreateCurrentFileHandleImpl(is_fat)) || st_narc.ReadFrom(narc_path, CreateCurrentFileHandleImpl(is_fat), ntr::fs::FileCompression::LZ77)) {
                    LoadNitroFsFileBrowseMenu(std::addressof(st_narc), std::addressof(st_narc.nitro_fs), true);
                }
                else {
                    ShowOkDialog("Invalid archive...");
                    g_LoadedNARCs.pop();
                }
            }
        }

        void OnSDATSelect(const bool is_fat, const std::string &sdat_path) {
            const auto res = ShowOpenConfirmationDialog("sound data file");
            if(res == DialogResult::Yes) {
                LoadSDATEditMenu(sdat_path, CreateCurrentFileHandleImpl(is_fat));
            }
        }

        void OnBMGSelect(const bool is_fat, const std::string &bmg_path) {
            const auto res = ShowOpenConfirmationDialog("BMG file");
            if(res == DialogResult::Yes) {
                LoadBMGEditMenu(bmg_path, CreateCurrentFileHandleImpl(is_fat));
            }
        }

        bool ReadROMHeaderBanner(const bool is_fat, const std::string &path, ntr::fmt::ROM &out_rom) {
            // Helper function to avoid reading the entire ROM nitrofs if we just want the header and the banner
            ntr::fs::BinaryFile bf;
            if(!bf.Open(CreateCurrentFileHandleImpl(is_fat), path, ntr::fs::OpenMode::Read)) {
                return false;
            }
            if(!bf.Read(out_rom.header)) {
                return false;
            }
            if(!bf.SetAbsoluteOffset(out_rom.header.banner_offset)) {
                return false;
            }
            if(!bf.Read(out_rom.banner)) {
                return false;
            }
            return true;
        }

        void OnItemFocus(const bool is_fat, const std::string &path, const bool is_rom) {
            CleanTextIcon(true);
            if(is_rom) {
                ntr::fmt::ROM rom = {};
                if(ReadROMHeaderBanner(is_fat, path, rom)) {
                    ntr::gfx::abgr8888::Color *icon_gfx;
                    if(ntr::gfx::ConvertBannerIconToRgba(rom.banner.icon_chr, rom.banner.icon_plt, icon_gfx)) {
                        g_CurrentROMIconGfx = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(ntr::gfx::IconWidth * ntr::gfx::IconHeight);
                        for(u32 i = 0; i < ntr::gfx::IconWidth * ntr::gfx::IconHeight; i++) {
                            g_CurrentROMIconGfx[i].raw_val = ntr::gfx::ConvertColor<ntr::gfx::abgr8888::Color, ntr::gfx::abgr1555::Color>(icon_gfx[i]).raw_val;
                        }
                        delete[] icon_gfx;
                        ASSERT_TRUE(g_CurrentROMIcon.CreateLoadFrom(false, 0, 40, g_CurrentROMIconGfx, ntr::gfx::IconWidth, ntr::gfx::IconHeight, false));
                        g_CurrentROMIcon.CenterXInScreen();

                        const auto sys_lang = GetSystemLanguage();
                        const auto title_text = ntr::util::ConvertFromUnicode(rom.banner.game_titles[static_cast<u32>(sys_lang)]);
                        ASSERT_TRUE(GetTextFont().RenderText(title_text, 0, 40 + 32 + 8, NormalLineHeight, ScreenWidth, false, ntr::gfx::abgr8888::BlackColor, true, g_CurrentROMText));
                        ASSERT_TRUE(g_CurrentROMText.Load());
                        g_CurrentROMText.CenterXInScreen();
                    }
                }
            }
        }

        void OnOtherStdioInput(const u32 k_down) {
            if(k_down & KEY_B) {
                CleanTextIcon(true);
                if(g_CurrentDirectory.empty()) {
                    LoadMainMenu();
                }
                else {
                    const auto new_path = ntr::fs::GetAbsolutePath(g_CurrentDirectory + "/..");
                    LoadStdioFileBrowseMenu(new_path);
                }
            }
        }

        void RestoreNitroFsFileBrowseMenuImpl() {
            const auto &entry = g_NitroFsEntryStack.top();
            g_NitroFsEntryStack.pop();
            LoadNitroFsFileBrowseMenu(entry.file_ref_ptr, entry.dir_ref_ptr, entry.is_loaded_narc, entry.path);
        }

        void OnOtherNitroFsInput(const u32 k_down) {
            if(k_down & KEY_B) {
                const auto &entry = g_NitroFsEntryStack.top();
                if(entry.is_loaded_narc) {
                    const auto res = ShowSaveConfirmationDialog();
                    switch(res) {
                        case DialogResult::Cancel: {
                            return;
                        }
                        case DialogResult::Yes: {
                            g_NitroFsEntryStack.pop();
                            auto &narc = g_LoadedNARCs.top();
                            SaveExternalFsFile(narc, "archive");
                            break;
                        }
                        case DialogResult::No: {
                            g_NitroFsEntryStack.pop();
                            break;
                        }
                    }
                    g_LoadedNARCs.pop();
                }
                else {
                    g_NitroFsEntryStack.pop();
                }
                CleanTextIcon(true);
                if(g_NitroFsEntryStack.empty()) {
                    if(HasLoadedROM()) {
                        RestoreROMEditMenu();
                    }
                    else {
                        RestoreFileBrowseMenu();
                    }
                }
                else {
                    const auto &cur_entry = g_NitroFsEntryStack.top();
                    if((cur_entry.file_ref_ptr != entry.file_ref_ptr) && !entry.is_loaded_narc) {
                        RestoreROMEditMenu();
                    }
                    else {
                        // Twice, to also remove the new one which will be added afterwards
                        g_CurrentDirectory = ntr::fs::RemoveLastEntry(g_CurrentDirectory);
                        g_CurrentDirectory = ntr::fs::RemoveLastEntry(g_CurrentDirectory);
                        RestoreNitroFsFileBrowseMenuImpl();
                    }
                }
            }
        }

        inline std::string MakePathFor(const std::string &file) {
            if(g_CurrentDirectory.empty()) {
                return file;
            }
            else {
                return g_CurrentDirectory + "/" + file;
            }
        }

        void LoadTextureImpl(const bool is_fat, const std::string &ncgr_name, const std::string &nclr_name, const std::string &nscr_name) {
            const auto ncgr_path = MakePathFor(ncgr_name);
            const auto nclr_path = MakePathFor(nclr_name);
            const auto nscr_path = MakePathFor(nscr_name);
            ntr::fmt::NCGR ncgr = {};
            ntr::fmt::NCLR nclr = {};
            ntr::fmt::NSCR nscr = {};
            const auto has_nscr = !nscr_name.empty();
            if(!ncgr.ReadFrom(ncgr_path, CreateCurrentFileHandleImpl(is_fat))) {
                ShowOkDialog("Invalid NCGR...");
                return;
            }
            if(!nclr.ReadFrom(nclr_path, CreateCurrentFileHandleImpl(is_fat))) {
                ShowOkDialog("Invalid NCLR...");
                return;
            }
            if(has_nscr) {
                if(!nscr.ReadFrom(nscr_path, CreateCurrentFileHandleImpl(is_fat))) {
                    ShowOkDialog("Invalid NSCR...");
                    return;
                }
            }
            const auto expected_single_plt_size = ntr::gfx::GetPaletteColorCountForPixelFormat(nclr.palette.pix_fmt) * sizeof(ntr::gfx::xbgr1555::Color);
            const auto plt_count = nclr.palette.data_size / expected_single_plt_size;
            if((plt_count > 1) && !has_nscr) {
                const auto res = ShowYesNoDialog("This NCLR seems to contain\nmore than one palette\n(it has " + std::to_string(plt_count) + " palettes)\n\nThis texture should be\nedited along with a NSCR file.\nAre you sure you want\nto proceed?");
                if(res != DialogResult::Yes) {
                    return;
                }
            }

            u32 def_width;
            u32 def_height;
            ncgr.ComputeDimensions(def_width, def_height);
            
            ntr::gfx::GraphicsToRgbaContext ctx = {
                .gfx_data = ncgr.data,
                .gfx_data_size = ncgr.char_data.data_size,
                .plt_data = nclr.data,
                .plt_data_size = nclr.palette.data_size,
                .def_width = def_width,
                .def_height = def_height,
                .pix_fmt = ncgr.char_data.pix_fmt,
                .char_fmt = ncgr.char_data.char_fmt,
                .first_color_transparent = true
            };
            if(has_nscr) {
                ctx.scr_data = nscr.data;
                ctx.scr_data_size = nscr.screen_data.data_size;
                ctx.scr_width = nscr.screen_data.screen_width;
                ctx.scr_height = nscr.screen_data.screen_height;
            }
            else {
                ctx.plt_idx = 0;
            }
            if(!ntr::gfx::ConvertGraphicsToRgba(ctx)) {
                ShowOkDialog("Unable to load texture...");
                return;
            }
            auto gfx_buf = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(ctx.out_width * ctx.out_height);
            for(u32 i = 0; i < ctx.out_width * ctx.out_height; i++) {
                gfx_buf[i].raw_val = ntr::gfx::ConvertColor<ntr::gfx::abgr8888::Color, ntr::gfx::abgr1555::Color>(ctx.out_rgba[i]).raw_val;
            }
            const auto plt_color_count = nclr.palette.data_size / sizeof(ntr::gfx::xbgr1555::Color);
            auto plt_copy = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(plt_color_count);
            const auto plt = reinterpret_cast<ntr::gfx::xbgr1555::Color*>(nclr.data);
            // First color transparent
            for(u32 i = 1; i < plt_color_count; i++) {
                plt_copy[i].raw_val = ntr::gfx::ConvertColor<ntr::gfx::xbgr1555::Color, ntr::gfx::abgr1555::Color>(plt[i]).raw_val;
            }
            if(ShowGraphicEditor(gfx_buf, plt_copy, plt_color_count, ctx.out_width, ctx.out_height)) {
                auto ok = false;
                ntr::gfx::RgbaToGraphicsContext ctx_2 = {
                    .rgba_data = ctx.out_rgba,
                    .width = ctx.out_width,
                    .height = ctx.out_height,
                    .pix_fmt = ncgr.char_data.pix_fmt,
                    .char_fmt = ncgr.char_data.char_fmt,
                    .gen_scr_data = has_nscr
                };
                RunWithDialog("Converting texture...", [&]() {
                    for(u32 i = 0; i < ctx.out_width * ctx.out_height; i++) {
                        ctx.out_rgba[i].raw_val = ntr::gfx::ConvertColor<ntr::gfx::abgr1555::Color, ntr::gfx::abgr8888::Color>(gfx_buf[i]).raw_val;
                    }
                    ok = ntr::gfx::ConvertRgbaToGraphics(ctx_2);
                });
                if(!ok) {
                    ShowOkDialog("Unable to convert texture...");
                    delete[] plt_copy;
                    delete[] gfx_buf;
                    delete[] ctx.out_rgba;
                    return;
                }

                auto old_gfx_data = ncgr.data;
                ncgr.data = ntr::util::NewArray<u8>(ctx_2.out_gfx_data_size);
                ncgr.char_data.data_size = ctx_2.out_gfx_data_size;
                delete[] old_gfx_data;
                std::memcpy(ncgr.data, ctx_2.out_gfx_data, ncgr.char_data.data_size);
                ok = SaveNormalFile(ncgr, "NCGR");
                if(!ok) {
                    delete[] plt_copy;
                    delete[] gfx_buf;
                    delete[] ctx.out_rgba;
                    delete[] ctx_2.out_gfx_data;
                    delete[] ctx_2.out_plt_data;
                    if(has_nscr) {
                        delete[] ctx_2.out_scr_data;
                    }
                    return;
                }

                auto old_plt_data = nclr.data;
                nclr.data = ntr::util::NewArray<u8>(ctx_2.out_plt_data_size);
                nclr.palette.data_size = ctx_2.out_plt_data_size;
                delete[] old_plt_data;
                std::memcpy(nclr.data, ctx_2.out_plt_data, nclr.palette.data_size);
                ok = SaveNormalFile(nclr, "NCLR");
                if(!ok) {
                    delete[] plt_copy;
                    delete[] gfx_buf;
                    delete[] ctx.out_rgba;
                    delete[] ctx_2.out_gfx_data;
                    delete[] ctx_2.out_plt_data;
                    if(has_nscr) {
                        delete[] ctx_2.out_scr_data;
                    }
                    return;
                }

                if(has_nscr) {
                    auto old_scr_data = nscr.data;
                    nscr.data = ntr::util::NewArray<u8>(ctx_2.out_scr_data_size);
                    nscr.screen_data.data_size = ctx_2.out_scr_data_size;
                    delete[] old_scr_data;
                    std::memcpy(nscr.data, ctx_2.out_scr_data, nscr.screen_data.data_size);
                    ok = SaveNormalFile(nscr, "NSCR");
                    if(!ok) {
                        delete[] plt_copy;
                        delete[] gfx_buf;
                        delete[] ctx.out_rgba;
                        delete[] ctx_2.out_gfx_data;
                        delete[] ctx_2.out_plt_data;
                        delete[] ctx_2.out_scr_data;
                        return;
                    }
                }
                
                if(has_nscr) {
                    ShowOkDialog("Saved texture in NCGR, NCLR, NSCR!");
                }
                else {
                    ShowOkDialog("Saved texture in NCGR, NCLR!");
                }
                delete[] ctx_2.out_gfx_data;
                delete[] ctx_2.out_plt_data;
                if(has_nscr) {
                    delete[] ctx_2.out_scr_data;
                }
            }
            delete[] plt_copy;
            delete[] gfx_buf;
            delete[] ctx.out_rgba;
        }

        void OnSelect(const bool is_fat, const std::vector<std::string> &selected_items) {
            if(selected_items.size() >= 2) {
                std::string ncgr;
                std::string nclr;
                std::string nscr;
                auto ok = true;
                for(const auto &sel_item : selected_items) {
                    const auto sel_ext = ntr::util::ToLowerString(ntr::fs::GetFileExtension(sel_item));
                    if(sel_ext == "ncgr") {
                        if(!ncgr.empty()) {
                            ok = false;
                            break;
                        }
                        ncgr = sel_item;
                    }
                    else if(sel_ext == "nclr") {
                        if(!nclr.empty()) {
                            ok = false;
                            break;
                        }
                        nclr = sel_item;
                    }
                    else if(sel_ext == "nscr") {
                        if(!nscr.empty()) {
                            ok = false;
                            break;
                        }
                        nscr = sel_item;
                    }
                }
                if(ok) {
                    if(!ncgr.empty() && !nclr.empty()) {
                        LoadTextureImpl(is_fat, ncgr, nclr, nscr);
                        return;
                    }
                }
            }
            ShowOkDialog("Supported selections:\n\nNCGR + NCLR\nNCGR + NCLR + NSCR");
        }

        void LoadFileEntryImpl(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
            const auto ext = ntr::util::ToLowerString(ntr::fs::GetFileExtension(entry_name));
            // TODO: detect formats by checking headers, etc.
            if((ext == "nds") || (ext == "srl") || (ext == "ids") || (ext == "dsi") || (ext == "app")) {
                entries.push_back({
                    .icon_gfx = g_ROMIconGfx,
                    .text = entry_name,
                    .on_select = std::bind(OnROMSelect, is_fat, entry_path),
                    .on_focus = std::bind(OnItemFocus, is_fat, entry_path, true)
                });
            }
            else if((ext == "arc") || (ext == "narc") || (ext == "carc")) {
                entries.push_back({
                    .icon_gfx = GetFsIcon(),
                    .text = entry_name,
                    .on_select = std::bind(OnNARCSelect, is_fat, entry_path),
                    .on_focus = std::bind(OnItemFocus, is_fat, entry_path, false)
                });
            }
            else if(ext == "sdat") {
                entries.push_back({
                    .icon_gfx = g_MusicIconGfx,
                    .text = entry_name,
                    .on_select = std::bind(OnSDATSelect, is_fat, entry_path),
                    .on_focus = std::bind(OnItemFocus, is_fat, entry_path, false)
                });
            }
            else if(ext == "ncgr") {
                entries.push_back({
                    .icon_gfx = GetGfxIcon(),
                    .text = entry_name,
                    .on_select = std::bind(OnFileSelect, is_fat, entry_path),
                    .on_focus = std::bind(OnItemFocus, is_fat, entry_path, false)
                });
            }
            else if((ext == "nclr") || (ext == "nscr")) {
                entries.push_back({
                    .icon_gfx = g_TexIconGfx,
                    .text = entry_name,
                    .on_select = std::bind(OnFileSelect, is_fat, entry_path),
                    .on_focus = std::bind(OnItemFocus, is_fat, entry_path, false)
                });
            }
            else if(ext == "bmg") {
                entries.push_back({
                    .icon_gfx = GetTextIcon(),
                    .text = entry_name,
                    .on_select = std::bind(OnBMGSelect, is_fat, entry_path),
                    .on_focus = std::bind(OnItemFocus, is_fat, entry_path, false)
                });
            }
            else {
                entries.push_back({
                    .icon_gfx = g_FileIconGfx,
                    .text = entry_name,
                    .on_select = std::bind(OnFileSelect, is_fat, entry_path),
                    .on_focus = std::bind(OnItemFocus, is_fat, entry_path, false)
                });
            }
        }

    }

    void InitializeFileBrowseMenu() {
        u32 w;
        u32 h;
        ASSERT_TRUE(LoadPNGGraphics("ui/dir_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_DirectoryIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_TRUE(LoadPNGGraphics("ui/file_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_FileIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_TRUE(LoadPNGGraphics("ui/rom_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_ROMIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_TRUE(LoadPNGGraphics("ui/music_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_MusicIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_TRUE(LoadPNGGraphics("ui/tex_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_TexIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);
    }

    void LoadStdioFileBrowseMenu(const std::string &base_dir) {
        g_CurrentDirectory = base_dir;
        std::vector<ScrollMenuEntry> entries;
		FS_FOR_EACH_STDIO_ENTRY(base_dir, {
            if((entry_name == ".") || (entry_name == "..")) {
                continue;
            }
            if(is_dir) {
                entries.push_back({
                    .icon_gfx = g_DirectoryIconGfx,
                    .text = entry_name,
                    .on_select = std::bind(OnStdioDirectorySelect, entry_path),
                    .on_focus = std::bind(OnItemFocus, true, entry_path, false)
                });
            }
            else if(is_file) {
                LoadFileEntryImpl(true, entry_name, entry_path, entries);
            }
        });

        LoadScrollMenu(entries, OnOtherStdioInput);
        EnableScrollMenuSelection(std::bind(OnSelect, true, std::placeholders::_1));
	}

    void LoadNitroFsFileBrowseMenu(ntr::fmt::nfs::NitroFsFileFormat *file_ref_ptr, ntr::fmt::nfs::NitroDirectory *dir_ref_ptr, const bool is_narc, const std::string &base_path) {
        const NitroFsBrowseEntry entry = {
            .file_ref_ptr = file_ref_ptr,
            .dir_ref_ptr = dir_ref_ptr,
            .is_loaded_narc = is_narc,
            .path = base_path
        };
        g_NitroFsEntryStack.push(std::move(entry));
        std::vector<ScrollMenuEntry> entries;
        if(!file_ref_ptr->DoWithReadFile([&](ntr::fs::BinaryFile &bf) {
            if(base_path.empty()) {
                if(dir_ref_ptr->is_root) {
                    g_CurrentDirectory = "";
                }
                else {
                    if(!g_CurrentDirectory.empty()) {
                        g_CurrentDirectory += "/";
                    }
                    g_CurrentDirectory += dir_ref_ptr->GetName(bf);
                }
                auto &self_entry = g_NitroFsEntryStack.top();
                self_entry.path = g_CurrentDirectory;
            }
            else {
                g_CurrentDirectory = base_path;
            }
            for(auto &subdir : dir_ref_ptr->dirs) {
                const auto entry_name = subdir.GetName(bf);
                auto entry_path = g_CurrentDirectory;
                if(!entry_path.empty()) {
                    entry_path += "/";
                }
                entry_path += entry_name;
                entries.push_back( ScrollMenuEntry {
                    .icon_gfx = g_DirectoryIconGfx,
                    .text = subdir.GetName(bf),
                    .on_select = std::bind(OnNitroFsDirectorySelect, file_ref_ptr, std::addressof(subdir)),
                    .on_focus = std::bind(OnItemFocus, true, entry_path, false)
                });
            }
            for(const auto &subfile : dir_ref_ptr->files) {
                const auto entry_name = subfile.GetName(bf);
                auto entry_path = g_CurrentDirectory;
                if(!entry_path.empty()) {
                    entry_path += "/";
                }
                entry_path += entry_name;
                LoadFileEntryImpl(false, entry_name, entry_path, entries);
            }
        })) {
            ShowOkDialog("Error reading NitroFs...");
            g_NitroFsEntryStack.pop();
            return;
        }

        LoadScrollMenu(entries, OnOtherNitroFsInput);
        EnableScrollMenuSelection(std::bind(OnSelect, false, std::placeholders::_1));
    }

    void RestoreFileBrowseMenu() {
        if(g_NitroFsEntryStack.empty()) {
            LoadStdioFileBrowseMenu(g_CurrentDirectory);
        }
        else {
            RestoreNitroFsFileBrowseMenuImpl();
        }
    }

    void UpdateFileBrowseMenuGraphics() {
        g_CurrentROMIcon.Update();
        g_CurrentROMText.Update();
    }

    bool IsInNitroFs() {
        return !g_NitroFsEntryStack.empty();
    }

    bool IsPreviousInNitroFs() {
        return g_NitroFsEntryStack.size() >= 2;
    }

    std::shared_ptr<ntr::fs::FileHandle> CreateCurrentFileHandle() {
        return CreateCurrentFileHandleImpl(!IsInNitroFs());
    }

    bool SaveFile(const std::string &file_path, const std::string &fmt, std::function<bool(const std::string&, bool)> on_save) {
        const auto prev_is_fat = !IsInNitroFs();
        auto path_copy = file_path;
        if(prev_is_fat) {
            ShowOkDialog("Write the new file name");
            const KeyboardContext ctx = {
                .only_numeric = false,
                .newline_allowed = false,
                .max_len = PATH_MAX,
            };
            const auto file_name = ntr::fs::GetFileName(file_path);
            auto file_name_copy = file_name;
            if(!ShowKeyboard(ctx, file_name_copy)) {
                return false;
            }
            if(file_name_copy == file_name) {
                const auto res_2 = ShowYesNoDialog("Are you sure you want to\noverwrite the original file?");
                if(res_2 != DialogResult::Yes) {
                    return false;
                }
            }
            path_copy = ntr::fs::GetBaseDirectory(file_path) + "/" + file_name_copy;
        }
        auto ok = false;
        RunWithDialog("Saving " + fmt + "...", [&]() {
            ok = on_save(path_copy, prev_is_fat);
        });
        if(ok) {
            ShowOkDialog("Successfully saved " + fmt + "!\n(" + path_copy + ")");
        }
        else {
            ShowOkDialog("Unable to save " + fmt + "...");
        }
        return ok;
    }

    ntr::gfx::abgr1555::Color *GetCurrentROMIcon() {
        return g_CurrentROMIconGfx;
    }

    ntr::gfx::abgr1555::Color *GetMusicIcon() {
        return g_MusicIconGfx;
    }

}
