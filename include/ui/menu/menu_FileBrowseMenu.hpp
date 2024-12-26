
#pragma once
#include <ui/menu/menu_Base.hpp>

namespace ui::menu {

    enum class FileSystemKind {
        ROM,
        NARC,
        Utility
    };

    void InitializeFileBrowseMenu();
    void LoadStdioFileBrowseMenu(const std::string &base_dir = "");
    void LoadNitroFsFileBrowseMenu(std::shared_ptr<ntr::fmt::nfs::NitroFsFileFormat> file_ref, ntr::fmt::nfs::NitroDirectory *dir_ref_ptr, const FileSystemKind fs_kind, const std::string &base_path = "");
    void RestoreFileBrowseMenu();
    void UpdateFileBrowseMenuGraphics();
    bool IsInNitroFs();
    bool IsPreviousInNitroFs();
    std::shared_ptr<ntr::fs::FileHandle> CreateCurrentFileHandle();
    ntr::Result SaveFile(const std::string &file_path, const std::string &fmt, std::function<ntr::Result(const std::string&, const bool)> on_save);

    template<typename T>
    inline ntr::Result SaveExternalFsFile(std::shared_ptr<T> &file, const std::string &fmt) {
        static_assert(std::is_base_of_v<ntr::fs::ExternalFsFileFormat, T>);

        return SaveFile(file->read_path, fmt, [&](const std::string &path, const bool is_fat) {
            if(is_fat) {
                NTR_R_TRY(file->WriteTo(path, std::make_shared<ntr::fs::StdioFileHandle>()));
            }
            return file->SaveFileSystem();
        });
    }

    inline ntr::Result SaveNormalFile(ntr::fs::FileFormat &file, const std::string &fmt) {
        return SaveFile(file.read_path, fmt, [&](const std::string &path, bool is_fat) {
            if(is_fat) {
                NTR_R_TRY(file.WriteTo(path, std::make_shared<ntr::fs::StdioFileHandle>()));
            }
            else {
                NTR_R_TRY(file.WriteTo(CreateCurrentFileHandle()));
            }
            NTR_R_SUCCEED();
        });
    }

    ntr::gfx::abgr1555::Color *GetCurrentROMIcon();
    ntr::gfx::abgr1555::Color *GetMusicIcon();

}