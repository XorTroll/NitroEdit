#include <ntr/fmt/fmt_NARC.hpp>

namespace ntr::fmt {

    Result NARC::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultNARCInvalidHeader);
        }

        NTR_R_TRY(bf.Read(this->fat));
        if(!this->fat.IsValid()) {
            NTR_R_FAIL(ResultNARCInvalidFileAllocationTableBlock);
        }

        const auto fat_entries_size = this->fat.entry_count * sizeof(nfs::FileAllocationTableEntry);
        NTR_R_TRY(bf.MoveOffset(fat_entries_size));

        NTR_R_TRY(bf.Read(this->fnt));
        if(!this->fnt.IsValid()) {
            NTR_R_FAIL(ResultNARCInvalidFileNameTableBlock);
        }

        const auto fimg_offset = util::AlignUp(this->header.header_size + this->fat.block_size + this->fnt.block_size, 0x4);
        NTR_R_TRY(bf.SetAbsoluteOffset(fimg_offset));
        NTR_R_TRY(bf.Read(this->fimg));
        if(!this->fimg.IsValid()) {
            NTR_R_FAIL(ResultNARCInvalidFileImageBlock);
        }

        NTR_R_SUCCEED();
    }

    Result NARC::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        this->nitro_fs = {};

        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        const auto fat_entries_size = this->fat.entry_count * sizeof(nfs::FileAllocationTableEntry);
        const auto fat_entries_offset = sizeof(Header) + sizeof(FileAllocationTableBlock);
        const auto fnt_entries_offset = fat_entries_offset + fat_entries_size + sizeof(FileNameTableBlock);
        NTR_R_TRY(nfs::ReadNitroFsFrom(fat_entries_offset, fnt_entries_offset, bf, this->nitro_fs));

        NTR_R_SUCCEED();
    }

}