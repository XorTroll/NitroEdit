
#pragma once
#include <ntr/fmt/fmt_Common.hpp>
#include <ntr/fmt/nfs/nfs_NitroFs.hpp>

namespace ntr::fmt {

    struct NARC : public nfs::NitroFsFileFormat {

        struct Header : public CommonHeader<0x4352414E /* "NARC" */ > {
        };

        struct FileAllocationTableBlock : public CommonBlock<0x46415442 /* "BTAF" */ > {
            u16 entry_count;
            u8 reserved[2];
        };

        struct FileNameTableBlock : public CommonBlock<0x464E5442 /* "BTNF" */ > {
        };

        struct FileImageBlock : public CommonBlock<0x46494D47 /* "GMIF" */ > {
        };

        Header header;
        FileAllocationTableBlock fat;
        FileNameTableBlock fnt;
        FileImageBlock fimg;

        NARC() {}
        NARC(const NARC&) = delete;

        size_t GetBaseOffset() override {
            // Offset after GMIF header, where all file data starts
            return util::AlignUp(this->header.header_size + this->fat.block_size + this->fnt.block_size, 0x4) + sizeof(FileImageBlock);
        }

        size_t GetFatEntriesOffset() const override {
            return sizeof(Header) + sizeof(FileAllocationTableBlock);
        }

        size_t GetFatEntryCount() const override {
            return this->fat.entry_count;
        }

        Result OnFileSystemWrite(fs::BinaryFile &w_bf, const ssize_t size_diff) override {
            this->header.file_size += size_diff;
            this->fimg.block_size += size_diff;

            NTR_R_TRY(w_bf.SetAbsoluteOffset(0));
            NTR_R_TRY(w_bf.Write(this->header));

            const auto fimg_offset = this->header.header_size + this->fat.block_size + this->fnt.block_size;
            NTR_R_TRY(w_bf.SetAbsoluteOffset(fimg_offset));
            NTR_R_TRY(w_bf.Write(this->fimg));

            NTR_R_SUCCEED();
        }
        
        Result ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        Result ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
    };

    // Note: a CARC file is basically an LZ-compressed NARC

    using NARCFileHandle = nfs::NitroFsFileHandle<NARC>;

}