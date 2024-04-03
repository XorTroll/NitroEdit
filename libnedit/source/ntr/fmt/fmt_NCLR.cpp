#include <ntr/fmt/fmt_NCLR.hpp>

namespace ntr::fmt {

    Result NCLR::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultNCLRInvalidHeader);
        }

        NTR_R_TRY(bf.Read(this->palette));
        if(!this->palette.IsValid()) {
            NTR_R_FAIL(ResultNCLRInvalidPaletteDataBlock);
        }

        NTR_R_SUCCEED();
    }
    
    Result NCLR::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.SetAbsoluteOffset(sizeof(Header) + sizeof(PaletteDataBlock)));
        this->data = util::NewArray<u8>(this->palette.data_size);
        ScopeGuard on_fail_cleanup([&]() {
            delete[] this->data;
        });

        NTR_R_TRY(bf.ReadDataExact(this->data, this->palette.data_size));

        on_fail_cleanup.Cancel();
        NTR_R_SUCCEED();
    }

    Result NCLR::WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Write, comp));

        NTR_R_TRY(bf.SetAbsoluteOffset(sizeof(Header) + sizeof(PaletteDataBlock)));
        NTR_R_TRY(bf.WriteData(this->data, this->palette.data_size));

        this->palette.block_size = sizeof(PaletteDataBlock) + this->palette.data_size;
        this->header.file_size = sizeof(Header) + this->palette.block_size;

        NTR_R_TRY(bf.SetAbsoluteOffset(0));
        NTR_R_TRY(bf.Write(this->header));
        NTR_R_TRY(bf.Write(this->palette));

        NTR_R_SUCCEED();
    }

}