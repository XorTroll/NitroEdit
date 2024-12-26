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
#include <ntr/fmt/fmt_BMG.hpp>
#include <ntr/fmt/fmt_Utility.hpp>
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
            std::shared_ptr<ntr::fmt::nfs::NitroFsFileFormat> file_ref;
            ntr::fmt::nfs::NitroDirectory *dir_ref_ptr;

            FileSystemKind fs_kind;
            std::string path;

            inline bool IsROM() const {
                return this->fs_kind == FileSystemKind::ROM;
            }
        };

        std::stack<NitroFsBrowseEntry> g_NitroFsEntryStack;

        std::string g_CurrentDirectory;

        inline std::shared_ptr<ntr::fs::FileHandle> CreateCurrentNitroFsEntryFileHandle() {
            const auto &cur_entry = g_NitroFsEntryStack.top();
            switch(cur_entry.fs_kind) {
                case FileSystemKind::ROM: {
                    auto rom = std::reinterpret_pointer_cast<ntr::fmt::ROM>(cur_entry.file_ref);
                    return std::move(std::make_shared<ntr::fmt::ROMFileHandle>(rom));
                }
                case FileSystemKind::NARC: {
                    auto narc = std::reinterpret_pointer_cast<ntr::fmt::NARC>(cur_entry.file_ref);
                    return std::move(std::make_shared<ntr::fmt::NARCFileHandle>(narc));
                }
                case FileSystemKind::Utility: {
                    auto utility = std::reinterpret_pointer_cast<ntr::fmt::Utility>(cur_entry.file_ref);
                    return std::move(std::make_shared<ntr::fmt::UtilityFileHandle>(utility));
                }
                default: {
                    __builtin_unreachable();
                }
            }
        }

        inline std::shared_ptr<ntr::fs::FileHandle> CreateCurrentFileHandleImpl(const bool is_fat) {
            if(is_fat) {
                return std::move(std::make_shared<ntr::fs::StdioFileHandle>());
            }
            else {
                return CreateCurrentNitroFsEntryFileHandle();
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

        void OnNitroFsDirectorySelect(std::shared_ptr<ntr::fmt::nfs::NitroFsFileFormat> file_ref, ntr::fmt::nfs::NitroDirectory *dir) {
            const auto &cur_entry = g_NitroFsEntryStack.top();
            LoadNitroFsFileBrowseMenu(file_ref, dir, cur_entry.fs_kind);
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
                ASSERT_SUCCESSFUL(g_CurrentROMIcon.EnsureLoaded());
                ASSERT_SUCCESSFUL(g_CurrentROMText.EnsureLoaded());
            }
        }

        void OnNARCSelect(const bool is_fat, const std::string &narc_path) {
            const auto res = ShowOpenConfirmationDialog("archive");
            if(res == DialogResult::Yes) {
                auto narc = std::make_shared<ntr::fmt::NARC>();

                auto handle = CreateCurrentFileHandleImpl(is_fat);
                if(narc->ReadFrom(narc_path, handle).IsSuccess() || narc->ReadFrom(narc_path, handle, ntr::fs::FileCompression::LZ77).IsSuccess()) {
                    LoadNitroFsFileBrowseMenu(narc, std::addressof(narc->nitro_fs), FileSystemKind::NARC);
                }
                else {
                    ShowOkDialog("Invalid archive...");
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

        void OnUtilitySelect(const bool is_fat, const std::string &utility_path) {
            const auto res = ShowOpenConfirmationDialog("DWC utility file");
            if(res == DialogResult::Yes) {
                auto utility = std::make_shared<ntr::fmt::Utility>();

                auto handle = CreateCurrentFileHandleImpl(is_fat);
                if(utility->ReadFrom(utility_path, handle).IsSuccess()) {
                    LoadNitroFsFileBrowseMenu(utility, std::addressof(utility->nitro_fs), FileSystemKind::Utility);
                }
                else {
                    ShowOkDialog("Invalid utility file...");
                }
            }
        }

        template<typename T>
        bool TryValidateFormat(const bool is_fat, const std::string &entry_path) {
            static_assert(std::is_base_of_v<ntr::fs::FileFormat, T>);

            const auto ext = ntr::fs::GetFileExtension(entry_path);

            auto fmt = T{};
            auto handle = CreateCurrentFileHandleImpl(is_fat);
            if(fmt.Validate(entry_path, handle).IsSuccess()) {
                return true;
            }
            else if(fmt.Validate(entry_path, handle, ntr::fs::FileCompression::LZ77).IsSuccess()) {
                return true;
            }
            else {
                return false;
            }
        }

        void OnItemFocus(const bool is_fat, const std::string &path, const bool is_rom) {
            CleanTextIcon(true);

            if(is_rom) {
                ntr::fmt::ROM rom = {};
                if(rom.Validate(path, CreateCurrentFileHandleImpl(is_fat)).IsSuccess()) {
                    ntr::gfx::abgr8888::Color *icon_gfx;
                    if(ntr::gfx::ConvertBannerIconToRgba(rom.banner.icon_chr, rom.banner.icon_plt, icon_gfx).IsSuccess()) {
                        g_CurrentROMIconGfx = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(ntr::gfx::IconWidth * ntr::gfx::IconHeight);
                        for(u32 i = 0; i < ntr::gfx::IconWidth * ntr::gfx::IconHeight; i++) {
                            g_CurrentROMIconGfx[i].raw_val = ntr::gfx::ConvertColor<ntr::gfx::abgr8888::Color, ntr::gfx::abgr1555::Color>(icon_gfx[i]).raw_val;
                        }
                        delete[] icon_gfx;
                        ASSERT_SUCCESSFUL(g_CurrentROMIcon.CreateLoadFrom(false, 0, 40, g_CurrentROMIconGfx, ntr::gfx::IconWidth, ntr::gfx::IconHeight, false));
                        g_CurrentROMIcon.CenterXInScreen();

                        const auto sys_lang = GetSystemLanguage();
                        const auto title_text = ntr::util::ConvertFromUnicode(rom.banner.game_titles[static_cast<u32>(sys_lang)]);
                        ASSERT_SUCCESSFUL(GetTextFont().RenderText(title_text, 0, 40 + 32 + 8, NormalLineHeight, ScreenWidth, false, ntr::gfx::abgr8888::BlackColor, true, g_CurrentROMText));
                        ASSERT_SUCCESSFUL(g_CurrentROMText.Load());
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
            LoadNitroFsFileBrowseMenu(entry.file_ref, entry.dir_ref_ptr, entry.fs_kind, entry.path);
        }

        void OnOtherNitroFsInput(const u32 k_down) {
            if(k_down & KEY_B) {
                const auto &entry = g_NitroFsEntryStack.top();

                if(!entry.IsROM()) {
                    const auto res = ShowSaveConfirmationDialog();
                    switch(res) {
                        case DialogResult::Cancel: {
                            return;
                        }
                        case DialogResult::Yes: {
                            g_NitroFsEntryStack.pop();
                            switch(entry.fs_kind) {
                                case FileSystemKind::NARC: {
                                    auto narc = std::static_pointer_cast<ntr::fmt::NARC>(entry.file_ref);
                                    SaveExternalFsFile(narc, "archive");
                                    break;
                                }
                                case FileSystemKind::Utility: {
                                    auto utility = std::static_pointer_cast<ntr::fmt::Utility>(entry.file_ref);
                                    SaveExternalFsFile(utility, "DWC utility file");
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                            break;
                        }
                        case DialogResult::No: {
                            g_NitroFsEntryStack.pop();
                            break;
                        }
                    }
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
                    if((cur_entry.file_ref != entry.file_ref) && entry.IsROM()) {
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
            auto rc = ncgr.ReadFrom(ncgr_path, CreateCurrentFileHandleImpl(is_fat));
            if(rc.IsFailure()) {
                ShowOkDialog("Invalid NCGR:\n'" + FormatResult(rc) + "'");
                return;
            }

            rc = nclr.ReadFrom(nclr_path, CreateCurrentFileHandleImpl(is_fat));
            if(rc.IsFailure()) {
                ShowOkDialog("Invalid NCLR:\n'" + FormatResult(rc) + "'");
                return;
            }

            if(has_nscr) {
                rc = nscr.ReadFrom(nscr_path, CreateCurrentFileHandleImpl(is_fat));
                if(rc.IsFailure()) {
                    ShowOkDialog("Invalid NSCR:\n'" + FormatResult(rc) + "'");
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

            rc = ntr::gfx::ConvertGraphicsToRgba(ctx);
            if(rc.IsFailure()) {
                ShowOkDialog("Unable to load texture:\n'" + FormatResult(rc) + "'");
                return;
            }
            ntr::ScopeGuard on_exit_cleanup_1([&]() {
                delete[] ctx.out_rgba;
            });

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
            ntr::ScopeGuard on_exit_cleanup_2([&]() {
                delete[] plt_copy;
                delete[] gfx_buf;
            });

            if(ShowGraphicEditor(gfx_buf, plt_copy, plt_color_count, ctx.out_width, ctx.out_height)) {
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
                    rc = ntr::gfx::ConvertRgbaToGraphics(ctx_2);
                });
                if(rc.IsFailure()) {
                    ShowOkDialog("Unable to convert texture:\n'" + FormatResult(rc) + "'");
                    return;
                }
                ntr::ScopeGuard on_exit_cleanup_3([&]() {
                    delete[] ctx_2.out_gfx_data;
                    delete[] ctx_2.out_plt_data;
                    if(has_nscr) {
                        delete[] ctx_2.out_scr_data;
                    }
                });

                auto old_gfx_data = ncgr.data;
                ncgr.data = ntr::util::NewArray<u8>(ctx_2.out_gfx_data_size);
                ncgr.char_data.data_size = ctx_2.out_gfx_data_size;
                delete[] old_gfx_data;
                std::memcpy(ncgr.data, ctx_2.out_gfx_data, ncgr.char_data.data_size);
                rc = SaveNormalFile(ncgr, "NCGR");
                if(rc.IsFailure()) {
                    ShowOkDialog("Unable to save NCGR file:\n'" + FormatResult(rc) + "'");
                    return;
                }

                auto old_plt_data = nclr.data;
                nclr.data = ntr::util::NewArray<u8>(ctx_2.out_plt_data_size);
                nclr.palette.data_size = ctx_2.out_plt_data_size;
                delete[] old_plt_data;
                std::memcpy(nclr.data, ctx_2.out_plt_data, nclr.palette.data_size);
                rc = SaveNormalFile(nclr, "NCLR");
                if(rc.IsFailure()) {
                    ShowOkDialog("Unable to save NCLR file:\n'" + FormatResult(rc) + "'");
                    return;
                }

                if(has_nscr) {
                    auto old_scr_data = std::move(nscr.data);
                    nscr.data = ntr::util::NewArray<u8>(ctx_2.out_scr_data_size);
                    nscr.screen_data.data_size = ctx_2.out_scr_data_size;
                    delete[] old_scr_data;
                    std::memcpy(nscr.data, ctx_2.out_scr_data, nscr.screen_data.data_size);
                    rc = SaveNormalFile(nscr, "NSCR");
                    if(rc.IsFailure()) {
                        ShowOkDialog("Unable to save NSCR file:\n'" + FormatResult(rc) + "'");
                        return;
                    }
                }
                
                if(has_nscr) {
                    ShowOkDialog("Saved texture in NCGR, NCLR, NSCR!");
                }
                else {
                    ShowOkDialog("Saved texture in NCGR, NCLR!");
                }
            }
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

        namespace {

            template<typename T>
            inline bool CheckFileFormatImpl(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries, ntr::gfx::abgr1555::Color *icon_gfx, void(*on_select)(const bool, const std::string&)) {
                static_assert(std::is_base_of_v<ntr::fs::FileFormat, T>);

                if(TryValidateFormat<T>(is_fat, entry_path)) {
                    entries.push_back({
                        .icon_gfx = icon_gfx,
                        .text = entry_name,
                        .on_select = std::bind(on_select, is_fat, entry_path),
                        .on_focus = std::bind(OnItemFocus, is_fat, entry_path, true)
                    });
                    return true;
                }
                else {
                    return false;
                }
            }

            inline bool CheckFileROM(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
                return CheckFileFormatImpl<ntr::fmt::ROM>(is_fat, entry_name, entry_path, entries, g_ROMIconGfx, OnROMSelect);
            }

            inline bool CheckFileNARC(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
                return CheckFileFormatImpl<ntr::fmt::NARC>(is_fat, entry_name, entry_path, entries, GetFsIcon(), OnNARCSelect);
            }

            inline bool CheckFileSDAT(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
                return CheckFileFormatImpl<ntr::fmt::SDAT>(is_fat, entry_name, entry_path, entries, g_MusicIconGfx, OnSDATSelect);
            }

            inline bool CheckFileNCGR(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
                return CheckFileFormatImpl<ntr::fmt::NCGR>(is_fat, entry_name, entry_path, entries, GetGfxIcon(), OnFileSelect);
            }

            inline bool CheckFileNCLR(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
                return CheckFileFormatImpl<ntr::fmt::NCLR>(is_fat, entry_name, entry_path, entries, g_TexIconGfx, OnFileSelect);
            }

            inline bool CheckFileNSCR(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
                return CheckFileFormatImpl<ntr::fmt::NSCR>(is_fat, entry_name, entry_path, entries, g_TexIconGfx, OnFileSelect);
            }

            inline bool CheckFileBMG(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
                return CheckFileFormatImpl<ntr::fmt::BMG>(is_fat, entry_name, entry_path, entries, GetTextIcon(), OnBMGSelect);
            }

            inline bool CheckFileUtility(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
                return CheckFileFormatImpl<ntr::fmt::Utility>(is_fat, entry_name, entry_path, entries, GetTextIcon(), OnUtilitySelect);
            }
        }

        void LoadFileEntryImpl(const bool is_fat, const std::string &entry_name, const std::string &entry_path, std::vector<ScrollMenuEntry> &entries) {
            // First deduce based on its extension to save time
            const auto entry_name_l = ntr::util::ToLowerString(entry_name);
            const auto ext = ntr::fs::GetFileExtension(entry_name_l);

            #define _UI_MENU_FILE_ENTRY_CHECK(check_fn) { \
                if(check_fn(is_fat, entry_name, entry_path, entries)) { \
                    return; \
                } \
            }

            auto not_rom = false;
            if((ext == "nds") || (ext == "srl")) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileROM);
                not_rom = true;
            }
            auto not_narc = false;
            if((ext == "narc") || (ext == "carc")) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileNARC);
                not_narc = true;
            }
            auto not_sdat = false;
            if(ext == "sdat") {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileSDAT);
                not_sdat = true;
            }
            auto not_ncgr = false;
            if(ext == "ncgr") {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileNCGR);
                not_ncgr = true;
            }
            auto not_nclr = false;
            if(ext == "nclr") {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileNCLR);
                not_nclr = true;
            }
            auto not_nscr = false;
            if(ext == "nscr") {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileNSCR);
                not_nscr = true;
            }
            auto not_bmg = false;
            if(ext == "bmg") {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileBMG);
                not_bmg = true;
            }
            auto not_utility = false;
            if(entry_name_l == "utility.bin") {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileUtility);
                not_utility = true;
            }

            // Then just try everything
            if(!not_rom) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileROM);
            }
            if(!not_narc) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileNARC);
            }
            if(!not_sdat) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileSDAT);
            }
            if(!not_ncgr) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileNCGR);
            }
            if(!not_nclr) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileNCLR);
            }
            if(!not_nscr) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileNSCR);
            }
            if(!not_bmg) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileBMG);
            }
            if(!not_utility) {
                _UI_MENU_FILE_ENTRY_CHECK(CheckFileUtility);
            }

            #undef _UI_MENU_FILE_ENTRY_CHECK

            // Fallthrough, unknown file
            entries.push_back({
                .icon_gfx = g_FileIconGfx,
                .text = entry_name,
                .on_select = std::bind(OnFileSelect, is_fat, entry_path),
                .on_focus = std::bind(OnItemFocus, is_fat, entry_path, false)
            });
        }

    }

    void InitializeFileBrowseMenu() {
        u32 w;
        u32 h;
        ASSERT_SUCCESSFUL(LoadPNGGraphics("ui/dir_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_DirectoryIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_SUCCESSFUL(LoadPNGGraphics("ui/file_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_FileIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_SUCCESSFUL(LoadPNGGraphics("ui/rom_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_ROMIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_SUCCESSFUL(LoadPNGGraphics("ui/music_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_MusicIconGfx));
        ASSERT_TRUE(w == ScrollMenuIconWidth);
        ASSERT_TRUE(h == ScrollMenuIconHeight);

        ASSERT_SUCCESSFUL(LoadPNGGraphics("ui/tex_icon.png", CreateSelfNitroFsFileHandle(), w, h, g_TexIconGfx));
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

    void LoadNitroFsFileBrowseMenu(std::shared_ptr<ntr::fmt::nfs::NitroFsFileFormat> file_ref, ntr::fmt::nfs::NitroDirectory *dir_ref_ptr, const FileSystemKind fs_kind, const std::string &base_path) {
        const NitroFsBrowseEntry entry = {
            .file_ref = file_ref,
            .dir_ref_ptr = dir_ref_ptr,
            .fs_kind = fs_kind,
            .path = base_path
        };
        g_NitroFsEntryStack.push(std::move(entry));
        std::vector<ScrollMenuEntry> entries;
        std::vector<std::pair<std::string, std::string>> load_file_entries;
        const auto rc = file_ref->DoWithReadFile([&](ntr::fs::BinaryFile &bf) {
            if(base_path.empty()) {
                if(dir_ref_ptr->is_root) {
                    g_CurrentDirectory = "";
                }
                else {
                    if(!g_CurrentDirectory.empty()) {
                        g_CurrentDirectory += "/";
                    }
                    std::string dir_name;
                    NTR_R_TRY(dir_ref_ptr->GetName(bf, dir_name));
                    g_CurrentDirectory += dir_name;
                }
                auto &self_entry = g_NitroFsEntryStack.top();
                self_entry.path = g_CurrentDirectory;
            }
            else {
                g_CurrentDirectory = base_path;
            }

            auto entry_path = g_CurrentDirectory;
            if(!entry_path.empty()) {
                entry_path += "/";
            }

            for(auto &subdir : dir_ref_ptr->dirs) {
                std::string entry_name;
                NTR_R_TRY(subdir.GetName(bf, entry_name));
                entries.push_back({
                    .icon_gfx = g_DirectoryIconGfx,
                    .text = entry_name,
                    .on_select = std::bind(OnNitroFsDirectorySelect, file_ref, std::addressof(subdir)),
                    .on_focus = std::bind(OnItemFocus, true, entry_path + entry_name, false)
                });
            }
            for(const auto &subfile : dir_ref_ptr->files) {
                std::string entry_name;
                NTR_R_TRY(subfile.GetName(bf, entry_name));
                load_file_entries.push_back({ entry_name, entry_path + entry_name });
            }
            NTR_R_SUCCEED();
        });
        if(rc.IsSuccess()) {
            for(const auto &[entry_name, entry_path] : load_file_entries) {
                LoadFileEntryImpl(false, entry_name, entry_path, entries);
            }
        }
        else {
            ShowOkDialog("Error reading NitroFs:\n'" + FormatResult(rc) + "'");
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

    ntr::Result SaveFile(const std::string &file_path, const std::string &fmt, std::function<ntr::Result(const std::string&, const bool)> on_save) {
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
        ntr::Result rc;
        RunWithDialog("Saving " + fmt + "...", [&]() {
            rc = on_save(path_copy, prev_is_fat);
        });
        if(rc.IsSuccess()) {
            ShowOkDialog("Successfully saved " + fmt + "!\n(" + path_copy + ")");
        }
        else {
            ShowOkDialog("Unable to save " + fmt + ":\n'" + FormatResult(rc) + "'");
        }
        return rc;
    }

    ntr::gfx::abgr1555::Color *GetCurrentROMIcon() {
        return g_CurrentROMIconGfx;
    }

    ntr::gfx::abgr1555::Color *GetMusicIcon() {
        return g_MusicIconGfx;
    }

}
