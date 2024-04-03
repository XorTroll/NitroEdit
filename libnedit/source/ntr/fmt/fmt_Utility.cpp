#include <ntr/fmt/fmt_Utility.hpp>

namespace ntr::fmt {

    Result Utility::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));

        size_t f_size;
        NTR_R_TRY(bf.GetSize(f_size));

        if((this->header.fat_offset > f_size) || ((this->header.fat_offset + this->header.fat_size) > f_size)) {
            return ResultUtilityInvalidSections;
        }
        if((this->header.fnt_offset > f_size) || ((this->header.fnt_offset + this->header.fnt_size) > f_size)) {
            return ResultUtilityInvalidSections;
        }

        NTR_R_SUCCEED();
    }

    Result Utility::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        this->nitro_fs = {};

        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(nfs::ReadNitroFsFrom(this->header.fat_offset, this->header.fnt_offset, bf, this->nitro_fs));

        NTR_R_SUCCEED();
    }

}