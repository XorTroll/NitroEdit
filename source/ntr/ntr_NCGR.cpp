#include <ntr/ntr_NCGR.hpp>

namespace ntr {

    bool NCGR::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
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

        if(!bf.Read(this->char_data)) {
            return false;
        }
        if(!this->char_data.IsValid()) {
            return false;
        }

        if(this->header.block_count >= 2) {
            if(!bf.MoveOffset(this->char_data.data_size)) {
                return false;
            }

            if(!bf.Read(this->char_pos)) {
                return false;
            }
            if(!this->char_pos.IsValid()) {
                return false;
            }
        }

        if(!bf.SetAbsoluteOffset(sizeof(Header) + sizeof(CharacterDataBlock))) {
            return false;
        }
        this->data = util::NewArray<u8>(this->char_data.data_size);
        if(!bf.ReadData(this->data, this->char_data.data_size)) {
            delete[] this->data;
            this->data = nullptr;
            return false;
        }

        return true;
    }

    bool NCGR::WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        if(!bf.Open(file_handle, path, fs::OpenMode::Write, comp)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(sizeof(Header) + sizeof(CharacterDataBlock))) {
            return false;
        }
        if(!bf.WriteData(this->data, this->char_data.data_size)) {
            return false;
        }

        this->char_data.block_size = sizeof(CharacterDataBlock) + this->char_data.data_size;
        this->header.file_size = sizeof(Header) + this->char_data.block_size;
        if(this->header.block_count >= 2) {
            this->header.file_size += sizeof(CharacterPositionBlock);
        }

        if(!bf.SetAbsoluteOffset(0)) {
            return false;
        }
        if(!bf.Write(this->header)) {
            return false;
        }

        if(!bf.Write(this->char_data)) {
            return false;
        }

        if(this->header.block_count >= 2) {
            if(!bf.MoveOffset(this->char_data.data_size)) {
                return false;
            }
            if(!bf.Write(this->char_pos)) {
                return false;
            }
        }

        return true;
    }

}