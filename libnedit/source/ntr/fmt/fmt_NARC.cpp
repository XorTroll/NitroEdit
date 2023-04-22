#include <ntr/fmt/fmt_NARC.hpp>

namespace ntr::fmt {

    bool NARC::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        this->nitro_fs = {};

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

        if(!bf.Read(this->fat)) {
            return false;
        }
        if(!this->fat.IsValid()) {
            return false;
        }

        const auto fat_entries_size = this->fat.entry_count * sizeof(nfs::FileAllocationTableEntry);
        if(!bf.MoveOffset(fat_entries_size)) {
            return false;
        }

        if(!bf.Read(this->fnt)) {
            return false;
        }
        if(!this->fnt.IsValid()) {
            return false;
        }

        const auto fimg_offset = this->header.header_size + this->fat.block_size + this->fnt.block_size;
        if(!bf.SetAbsoluteOffset(fimg_offset)) {
            return false;
        }
        if(!bf.Read(this->fimg)) {
            return false;
        }
        if(!this->fimg.IsValid()) {
            return false;
        }

        const auto fat_entries_offset = sizeof(Header) + sizeof(FileAllocationTableBlock);
        const auto fnt_entries_offset = fat_entries_offset + fat_entries_size + sizeof(FileNameTableBlock);

        if(!nfs::ReadNitroFsFrom(fat_entries_offset, fnt_entries_offset, bf, this->nitro_fs)) {
            return false;
        }

        return true;
    }

}