#include <ntr/fmt/fmt_NCGR.hpp>

namespace ntr::fmt {

    Result NCGR::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(this->header.magic);
        }

        NTR_R_TRY(bf.Read(this->char_data));
        if(!this->char_data.IsValid()) {
            NTR_R_FAIL(ResultNCGRInvalidCharacterDataBlock);
        }

        if(this->header.block_count >= 2) {
            NTR_R_TRY(bf.MoveOffset(this->char_data.data_size));

            NTR_R_TRY(bf.Read(this->char_pos));
            if(!this->char_pos.IsValid()) {
                NTR_R_FAIL(ResultNCGRInvalidCharacterPositionBlock);
            }
        }

        NTR_R_SUCCEED();
    }

    Result NCGR::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.SetAbsoluteOffset(sizeof(Header) + sizeof(CharacterDataBlock)));
        this->data = util::NewArray<u8>(this->char_data.data_size);
        ScopeGuard on_fail_cleanup([&]() {
            delete[] this->data;
        });

        NTR_R_TRY(bf.ReadDataExact(this->data, this->char_data.data_size));

        on_fail_cleanup.Cancel();
        NTR_R_SUCCEED();
    }

    Result NCGR::WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf;
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Write, comp));

        NTR_R_TRY(bf.SetAbsoluteOffset(sizeof(Header) + sizeof(CharacterDataBlock)));
        NTR_R_TRY(bf.WriteData(this->data, this->char_data.data_size));

        this->header.EnsureMagic();
        this->char_data.block_size = sizeof(CharacterDataBlock) + this->char_data.data_size;
        this->header.file_size = sizeof(Header) + this->char_data.block_size;
        if(this->header.block_count >= 2) {
            this->header.file_size += sizeof(CharacterPositionBlock);
        }

        NTR_R_TRY(bf.SetAbsoluteOffset(0));
        NTR_R_TRY(bf.Write(this->header));

        NTR_R_TRY(bf.Write(this->char_data));

        if(this->header.block_count >= 2) {
            NTR_R_TRY(bf.MoveOffset(this->char_data.data_size));
            NTR_R_TRY(bf.Write(this->char_pos));
        }

        NTR_R_SUCCEED();
    }

}