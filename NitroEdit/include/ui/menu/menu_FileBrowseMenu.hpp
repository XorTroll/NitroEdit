
#pragma once
#include <ui/menu/menu_Base.hpp>

namespace ui::menu {

    void InitializeFileBrowseMenu();
    void LoadStdioFileBrowseMenu(const std::string &base_dir = "");
    void LoadNitroFsFileBrowseMenu(ntr::fmt::nfs::NitroFsFileFormat *file_ref_ptr, ntr::fmt::nfs::NitroDirectory *dir_ref_ptr, const bool is_narc, const std::string &base_path = "");
    void RestoreFileBrowseMenu();
    void UpdateFileBrowseMenuGraphics();
    bool IsInNitroFs();
    bool IsPreviousInNitroFs();
    std::shared_ptr<ntr::fs::FileHandle> CreateCurrentFileHandle();
    bool SaveFile(const std::string &file_path, const std::string &fmt, std::function<bool(const std::string&, bool)> on_save);

    inline bool SaveExternalFsFile(ntr::fs::ExternalFsFileFormat &file, const std::string &fmt) {
        return SaveFile(file.read_path, fmt, [&](const std::string &path, bool is_fat) {
            if(is_fat) {
                if(!file.WriteTo(path, std::make_shared<ntr::fs::StdioFileHandle>())) {
                    return false;
                }
            }
            return file.SaveFileSystem();
        });
    }

    inline bool SaveNormalFile(ntr::fs::FileFormat &file, const std::string &fmt) {
        return SaveFile(file.read_path, fmt, [&](const std::string &path, bool is_fat) {
            if(is_fat) {
                if(!file.WriteTo(path, std::make_shared<ntr::fs::StdioFileHandle>())) {
                    return false;
                }
            }
            else {
                if(!file.WriteTo(CreateCurrentFileHandle())) {
                    return false;
                }
            }
            return true;
        });
    }

    ntr::gfx::abgr1555::Color *GetCurrentROMIcon();
    ntr::gfx::abgr1555::Color *GetMusicIcon();

}