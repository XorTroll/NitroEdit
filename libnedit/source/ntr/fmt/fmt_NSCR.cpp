#include <ntr/fmt/fmt_NSCR.hpp>

namespace ntr::fmt {

    bool NSCR::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        if(!bf.Open(file_handle, path, fs::OpenMode::Read, comp)) {
            return false;
        }

        if(!bf.Read(this->header)) {
            return false;
        }
        if(!this->header.IsValid()) {
            return false;
        }

        if(!bf.Read(this->screen_data)) {
            return false;
        }
        if(!this->screen_data.IsValid()) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(sizeof(Header) + sizeof(ScreenDataBlock))) {
            return false;
        }
        this->data = util::NewArray<u8>(this->screen_data.data_size);
        if(!bf.ReadData(this->data, this->screen_data.data_size)) {
            delete[] this->data;
            this->data = nullptr;
            return false;
        }

        return true;
    }

    bool NSCR::WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        /* TODO */
        return false;
    }

}