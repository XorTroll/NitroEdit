#include <ntr/fmt/fmt_STRM.hpp>

namespace ntr::fmt {

    bool STRM::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        if(!bf.Open(file_handle, path, fs::OpenMode::Read, comp)) {
            return false;
        }

        if(!bf.Read(this->header)) {
            return false;
        }
        if(!this->header.IsValid()) {
            return false;
        }

        if(!bf.Read(this->head)) {
            return false;
        }
        if(!this->head.IsValid()) {
            return false;
        }
        
        if(!bf.Read(this->data)) {
            return false;
        }
        if(!this->data.IsValid()) {
            return false;
        }

        return true;
    }

}