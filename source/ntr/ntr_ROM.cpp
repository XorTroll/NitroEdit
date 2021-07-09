#include <ntr/ntr_ROM.hpp>
#include <fs/fs_Fat.hpp>

namespace ntr {

    bool ROM::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        this->nitro_fs = {};

        fs::BinaryFile bf = {};
        if(!bf.Open(file_handle, path, fs::OpenMode::Read, comp)) {
            return false;
        }

        if(!bf.Read(this->header)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(this->header.banner_offset)) {
            return false;
        }
        if(!bf.Read(this->banner)) {
            return false;
        }

        if(!nfs::ReadNitroFsFrom(this->header.fat_offset, this->header.fnt_offset, bf, this->nitro_fs)) {
            return false;
        }

        return true;
    }

}