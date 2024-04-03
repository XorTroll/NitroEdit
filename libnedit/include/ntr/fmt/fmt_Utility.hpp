
#pragma once
#include <ntr/fmt/fmt_Common.hpp>
#include <ntr/fmt/nfs/nfs_NitroFs.hpp>

namespace ntr::fmt {

    struct Utility : public nfs::NitroFsFileFormat {

        struct Header {
            u32 fnt_offset;
            u32 fnt_size;
            u32 fat_offset;
            u32 fat_size;
        };

        Header header;

        Utility() {}
        Utility(const Utility&) = delete;

        size_t GetBaseOffset() override {
            // Offset after FAT and FNT (and alignment)
            return util::AlignUp(this->GetFatEntriesOffset() + this->header.fat_size, 0x20);
        }

        size_t GetFatEntriesOffset() override {
            return this->header.fat_offset;
        }

        size_t GetFatEntryCount() override {
            return this->header.fat_size / sizeof(nfs::FileAllocationTableEntry);
        }

        Result OnFileSystemWrite(fs::BinaryFile &w_bf, const ssize_t size_diff) override {
            // TODO
            NTR_R_SUCCEED();
        }
        
        Result ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        Result ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
    };

    using UtilityFileHandle = nfs::NitroFsFileHandle<Utility>;

}