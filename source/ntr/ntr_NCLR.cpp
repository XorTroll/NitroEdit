#include <ntr/ntr_NCLR.hpp>

namespace ntr {

    bool NCLR::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
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

        if(!bf.Read(this->palette)) {
            return false;
        }
        if(!this->palette.IsValid()) {
            return false;
        }

        this->data = util::NewArray<u8>(this->palette.data_size);
        if(!bf.ReadData(this->data, this->palette.data_size)) {
            delete[] this->data;
            this->data = nullptr;
            return false;
        }

        return true;
    }

    bool NCLR::WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        if(!bf.Open(file_handle, path, fs::OpenMode::Write, comp)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(sizeof(Header) + sizeof(PaletteDataBlock))) {
            return false;
        }
        if(!bf.WriteData(this->data, this->palette.data_size)) {
            return false;
        }

        this->palette.block_size = sizeof(PaletteDataBlock) + this->palette.data_size;
        this->header.file_size = sizeof(Header) + this->palette.block_size;

        if(!bf.SetAbsoluteOffset(0)) {
            return false;
        }
        if(!bf.Write(this->header)) {
            return false;
        }

        if(!bf.Write(this->palette)) {
            return false;
        }

        return true;
    }

}