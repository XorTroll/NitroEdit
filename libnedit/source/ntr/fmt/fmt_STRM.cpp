#include <ntr/fmt/fmt_STRM.hpp>

namespace ntr::fmt {

    Result STRM::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultSTRMInvalidHeader);
        }

        NTR_R_TRY(bf.Read(this->head));
        if(!this->head.IsValid()) {
            NTR_R_FAIL(ResultSTRMInvalidHeadSection);
        }
        
        NTR_R_TRY(bf.Read(this->data));
        if(!this->data.IsValid()) {
            NTR_R_FAIL(ResultSTRMInvalidDataSection);
        }

        NTR_R_SUCCEED();
    }
    
    Result STRM::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        // TODO: samples

        NTR_R_SUCCEED();
    }

}