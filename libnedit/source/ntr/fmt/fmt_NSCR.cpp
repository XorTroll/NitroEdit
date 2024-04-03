#include <ntr/fmt/fmt_NSCR.hpp>

namespace ntr::fmt {

    Result NSCR::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultNSCRInvalidHeader);
        }

        NTR_R_TRY(bf.Read(this->screen_data));
        if(!this->screen_data.IsValid()) {
            NTR_R_FAIL(ResultNSCRInvalidScreenDataBlock);
        }

        NTR_R_SUCCEED();
    }

    Result NSCR::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.SetAbsoluteOffset(sizeof(Header) + sizeof(ScreenDataBlock)));
        this->data = util::NewArray<u8>(this->screen_data.data_size);
        ScopeGuard on_fail_cleanup([&]() {
            delete[] this->data;
        });

        NTR_R_TRY(bf.ReadDataExact(this->data, this->screen_data.data_size));

        on_fail_cleanup.Cancel();
        NTR_R_SUCCEED();
    }

}