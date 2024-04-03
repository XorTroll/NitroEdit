#include <ntr/fs/fs_BinaryFile.hpp>

extern void Log(const std::string &log);

namespace ntr::fs {

    // Only way to handle compression is to read all the file and decompress it in an internal buffer.
    // Let's hope this doesn't make us run out of memory with big files.

    namespace {

        inline constexpr size_t GetAllocateSize(const size_t size) {
            size_t alloc_size = ReallocBufferSize;
            auto base_size = size;
            while(base_size > ReallocBufferSize) {
                alloc_size += ReallocBufferSize;
                base_size -= ReallocBufferSize;
            }
            return alloc_size;
        }

    }

    Result BinaryFile::LoadDecompressedData() {
        if(!this->IsCompressed()) {
            NTR_R_FAIL(ResultFileNotCompressed);
        }

        if(this->CanRead()) {
            size_t file_size;
            NTR_R_TRY(this->file_handle->GetSize(file_size));

            u32 lz_header;
            size_t read_size;
            NTR_R_TRY(this->file_handle->SetOffset(0, Position::Begin));
            NTR_R_TRY(this->file_handle->Read(&lz_header, sizeof(lz_header), read_size));
            if(read_size != sizeof(lz_header)) {
                NTR_R_FAIL(ResultUnexpectedReadSize);
            }

            switch(this->comp) {
                case FileCompression::LZ77: {
                    util::LzVersion dummy_ver;
                    NTR_R_TRY(util::LzValidateCompressed(lz_header, dummy_ver));
                    break;
                }
                default: {
                    break;
                }
            }

            auto enc_file_data = util::NewArray<u8>(file_size);
            ScopeGuard on_exit_cleanup([&]() {
                delete[] enc_file_data;
            });
            NTR_R_TRY(this->file_handle->SetOffset(0, Position::Begin));
            NTR_R_TRY(this->file_handle->Read(enc_file_data, file_size, read_size));
            if(read_size != file_size) {
                NTR_R_FAIL(ResultUnexpectedReadSize);
            }

            switch(this->comp) {
                case FileCompression::LZ77: {
                    size_t dummy_size;
                    NTR_R_TRY(util::LzDecompress(enc_file_data, this->dec_file_data, this->dec_file_size, this->comp_lz_ver, dummy_size));
                    break;
                }
                default: {
                    break;
                }
            }

            this->dec_file_buf_size = this->dec_file_size;
            this->dec_file_offset = 0;            
        }

        NTR_R_SUCCEED();
    }

    Result BinaryFile::SaveCompressedData() {
        if(!this->IsCompressed()) {
            NTR_R_FAIL(ResultFileNotCompressed);
        }
        if(!this->CanWrite()) {
            NTR_R_FAIL(ResultWriteNotSupported);
        }

        u8 *enc_file_data;
        size_t enc_file_data_size;
        switch(this->comp) {
            case FileCompression::LZ77: {
                NTR_R_TRY(util::LzCompressDefault(this->dec_file_data, this->dec_file_size, this->comp_lz_ver, enc_file_data, enc_file_data_size));
                break;
            }
            default: {
                break;
            }
        }
        ScopeGuard on_exit_cleanup([&]() {
            delete[] enc_file_data;
        });

        NTR_R_TRY(this->file_handle->SetOffset(0, Position::Begin));
        NTR_R_TRY(this->file_handle->Write(enc_file_data, enc_file_data_size));

        NTR_R_SUCCEED();
    }

    Result BinaryFile::ReadDecompressedData(void *read_buf, const size_t read_size, size_t &out_read_size) {
        auto actual_read_size = read_size;
        if((this->dec_file_offset + read_size) > this->dec_file_size) {
            actual_read_size = this->dec_file_size - this->dec_file_offset;
        }

        if(actual_read_size > 0) {
            std::memcpy(read_buf, this->dec_file_data + this->dec_file_offset, actual_read_size);
            this->dec_file_offset += actual_read_size;
            out_read_size = actual_read_size;
            NTR_R_SUCCEED();
        }
        else {
            NTR_R_FAIL(ResultEndOfData);
        }
    }

    Result BinaryFile::WriteDecompressedData(const void *write_buf, const size_t write_size) {
        if(this->dec_file_data == nullptr) {
            const auto new_size = GetAllocateSize(write_size);
            
            this->dec_file_data = util::NewArray<u8>(new_size);
            std::memcpy(this->dec_file_data, write_buf, write_size);
            this->dec_file_offset = write_size;
            this->dec_file_size = write_size;
            this->dec_file_buf_size = new_size;
            NTR_R_SUCCEED();
        }

        if((this->dec_file_offset + write_size) > this->dec_file_buf_size) {
            /* too small buffer, so let's realloc */
            NTR_R_TRY(this->ReallocateDecompressedData(this->dec_file_offset + write_size));
        }
        std::memcpy(this->dec_file_data + this->dec_file_offset, write_buf, write_size);
        this->dec_file_offset += write_size;
        if(this->dec_file_offset > this->dec_file_size) {
            this->dec_file_size = this->dec_file_offset;
        }
        NTR_R_SUCCEED();
    }

    Result BinaryFile::ReallocateDecompressedData(const size_t new_size) {
        if(!this->IsCompressed()) {
            NTR_R_FAIL(ResultFileNotCompressed);
        }

        const auto actual_new_size = GetAllocateSize(new_size);
        auto new_dec_file_data = util::NewArray<u8>(actual_new_size);
        std::memcpy(new_dec_file_data, this->dec_file_data, this->dec_file_offset);
        delete[] this->dec_file_data;
        this->dec_file_data = new_dec_file_data;
        this->dec_file_size = new_size;
        this->dec_file_buf_size = actual_new_size;
        NTR_R_SUCCEED();
    }

    Result BinaryFile::Open(std::shared_ptr<FileHandle> file_handle, const std::string &path, const OpenMode mode, const FileCompression comp) {
        this->Close();

        this->file_handle = file_handle;
        this->mode = mode;
        this->comp = comp;

        NTR_R_TRY(this->file_handle->Open(path, mode));
        
        if(this->IsCompressed()) {
            NTR_R_TRY(this->LoadDecompressedData());
        }

        this->ok = true;
        NTR_R_SUCCEED();
    }

    Result BinaryFile::Close() {
        if(this->IsValid()) {
            if(this->IsCompressed()) {
                ScopeGuard on_exit_cleanup([&]() {
                    delete[] this->dec_file_data;
                });

                if(this->CanWrite()) {
                    NTR_R_TRY(this->SaveCompressedData());
                }

                this->comp = FileCompression::None;
            }

            this->ok = false;
            NTR_R_TRY(this->file_handle->Close());
            NTR_R_SUCCEED();
        }
        else {
            NTR_R_FAIL(ResultInvalidFile);
        }
    }

    Result BinaryFile::CopyFrom(BinaryFile &other_bf, const size_t size) {
        auto copy_buf = util::NewArray<u8>(CopyBufferSize);
        ScopeGuard on_exit_cleanup([&]() {
            delete[] copy_buf;
        });

        auto cur_left_size = size;
        while(cur_left_size > 0) {
            const auto cur_copy_size = std::min(CopyBufferSize, cur_left_size);
            size_t read_size;
            NTR_R_TRY(other_bf.ReadData(copy_buf, cur_copy_size, read_size));
            NTR_R_TRY(this->WriteData(copy_buf, read_size));
            cur_left_size -= read_size;
        }
        
        NTR_R_SUCCEED();
    }

    Result BinaryFile::SetAbsoluteOffset(const size_t offset) {
        if(!this->IsValid()) {
            NTR_R_FAIL(ResultInvalidFile);
        }

        size_t cur_offset;
        NTR_R_TRY(this->GetAbsoluteOffset(cur_offset));
        if(cur_offset == offset) {
            NTR_R_SUCCEED();
        }

        if(this->IsCompressed()) {
            if(offset > this->dec_file_buf_size) {
                if(!this->CanWrite()) {
                    NTR_R_FAIL(ResultWriteNotSupported);
                }
                NTR_R_TRY(this->ReallocateDecompressedData(offset));
            }
            this->dec_file_offset = offset;
            if(offset > this->dec_file_size) {
                this->dec_file_size = offset;
            }
            NTR_R_SUCCEED();
        }
        else {
            return this->file_handle->SetOffset(offset, Position::Begin);
        }
    }

    Result BinaryFile::ReadData(void *read_buf, const size_t read_size, size_t &out_read_size) {
        if(!this->IsValid()) {
            NTR_R_FAIL(ResultInvalidFile);
        }
        if(!this->CanRead()) {
            NTR_R_FAIL(ResultReadNotSupported);
        }
        if(read_size == 0) {
            NTR_R_SUCCEED();
        }

        if(this->IsCompressed()) {
            return this->ReadDecompressedData(read_buf, read_size, out_read_size);
        }
        else {
            return this->file_handle->Read(read_buf, read_size, out_read_size);
        }
    }

    Result BinaryFile::WriteData(const void *write_buf, const size_t write_size) {
        if(!this->IsValid()) {
            NTR_R_FAIL(ResultInvalidFile);
        }
        if(!this->CanWrite()) {
            NTR_R_FAIL(ResultWriteNotSupported);
        }
        if(write_size == 0) {
            NTR_R_SUCCEED();
        }

        if(this->IsCompressed()) {
            return this->WriteDecompressedData(write_buf, write_size);
        }
        else {
            return this->file_handle->Write(write_buf, write_size);
        }
    }

}