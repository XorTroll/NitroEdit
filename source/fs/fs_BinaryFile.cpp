#include <fs/fs_BinaryFile.hpp>

namespace fs {

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

    bool BinaryFile::LoadDecompressedData() {
        if(!this->IsCompressed()) {
            return true;
        }

        if(this->CanRead()) {
            const auto file_sz = this->file_handle->GetSize();
            auto enc_file_data = util::NewArray<u8>(file_sz);
            
            if(!this->file_handle->SetOffset(0, Position::Begin)) {
                delete[] enc_file_data;
                return false;
            }
            if(!this->file_handle->Read(enc_file_data, file_sz)) {
                delete[] enc_file_data;
                return false;
            }

            switch(this->comp) {
                case FileCompression::LZ77: {
                    util::LZ77Decompress(this->dec_file_data, this->dec_file_size, enc_file_data);
                    break;
                }
                default: {
                    break;
                }
            }

            this->dec_file_buf_size = this->dec_file_size;
            this->dec_file_offset = 0;
            delete[] enc_file_data;
        }
        
        return true;
    }

    bool BinaryFile::SaveCompressedData() {
        if(!this->IsCompressed()) {
            return true;
        }

        if(this->CanWrite()) {
            u8 *enc_file_data;
            size_t enc_file_data_size;
            switch(this->comp) {
                case FileCompression::LZ77: {
                    util::LZ77Compress(enc_file_data, enc_file_data_size, this->dec_file_data, this->dec_file_size);
                    break;
                }
                default: {
                    break;
                }
            }

            if(!this->file_handle->SetOffset(0, Position::Begin)) {
                delete[] enc_file_data;
                return false;
            }
            if(!this->file_handle->Write(enc_file_data, enc_file_data_size)) {
                delete[] enc_file_data;
                return false;
            }

            delete[] enc_file_data;
        }
        
        return true;
    }

    bool BinaryFile::ReadDecompressedData(void *read_buf, const size_t read_size) {
        if((this->dec_file_offset + read_size) > this->dec_file_size) {
            return false;
        }

        std::memcpy(read_buf, this->dec_file_data + this->dec_file_offset, read_size);
        this->dec_file_offset += read_size;
        return true;
    }

    bool BinaryFile::WriteDecompressedData(const void *write_buf, const size_t write_size) {
        if(this->dec_file_data == nullptr) {
            const auto new_size = GetAllocateSize(write_size);
            this->dec_file_data = util::NewArray<u8>(new_size);
            std::memcpy(this->dec_file_data, write_buf, write_size);
            this->dec_file_offset = write_size;
            this->dec_file_size = write_size;
            this->dec_file_buf_size = new_size;
            return true;
        }

        if((this->dec_file_offset + write_size) > this->dec_file_buf_size) {
            /* too small buffer, so let's realloc */
            if(!this->ReallocateDecompressedData(this->dec_file_offset + write_size)) {
                return false;
            }
        }
        std::memcpy(this->dec_file_data + this->dec_file_offset, write_buf, write_size);
        this->dec_file_offset += write_size;
        if(this->dec_file_offset > this->dec_file_size) {
            this->dec_file_size = this->dec_file_offset;
        }
        return true;
    }

    bool BinaryFile::ReallocateDecompressedData(const size_t new_size) {
        if(!this->IsCompressed()) {
            return false;
        }

        const auto actual_new_size = GetAllocateSize(new_size);
        auto new_dec_file_data = util::NewArray<u8>(actual_new_size);
        std::memcpy(new_dec_file_data, this->dec_file_data, this->dec_file_offset);
        delete[] this->dec_file_data;
        this->dec_file_data = new_dec_file_data;
        this->dec_file_size = new_size;
        this->dec_file_buf_size = actual_new_size;
        return true;
    }

    bool BinaryFile::Open(std::shared_ptr<FileHandle> file_handle, const std::string &path, const OpenMode mode, const FileCompression comp) {
        this->Close();
        this->file_handle = file_handle;
        this->mode = mode;
        this->comp = comp;
        this->ok = this->file_handle->Open(path, mode) && this->LoadDecompressedData();
        return this->ok;
    }

    bool BinaryFile::Close() {
        if(this->IsValid()) {
            if(this->IsCompressed()) {
                if(!this->SaveCompressedData()) {
                    delete[] this->dec_file_data;
                    return false;
                }
                delete[] this->dec_file_data;
                this->comp = FileCompression::None;
            }
            const auto res = this->file_handle->Close();
            this->ok = false;
            return res;
        }
        return false;
    }

    bool BinaryFile::CopyFrom(BinaryFile &other_bf, const size_t size) {
        auto copy_buf = util::NewArray<u8>(CopyBufferSize);
        auto cur_left_size = size;
        while(cur_left_size > 0) {
            const auto cur_copy_size = std::min(CopyBufferSize, cur_left_size);
            if(!other_bf.ReadData(copy_buf, cur_copy_size)) {
                delete[] copy_buf;
                return false;
            }
            if(!this->WriteData(copy_buf, cur_copy_size)) {
                delete[] copy_buf;
                return false;
            }
            cur_left_size -= cur_copy_size;
        }
        delete[] copy_buf;
        return true;
    }

    bool BinaryFile::SetAbsoluteOffset(const size_t offset) {
        if(this->IsValid()) {
            if(this->GetAbsoluteOffset() == offset) {
                return true;
            }
            if(this->IsCompressed()) {
                if(offset > this->dec_file_buf_size) {
                    if(!this->CanWrite()) {
                        return false;
                    }
                    if(!this->ReallocateDecompressedData(offset)) {
                        return false;
                    }
                }
                this->dec_file_offset = offset;
                if(offset > this->dec_file_size) {
                    this->dec_file_size = offset;
                }
                return true;
            }
            else {
                return this->file_handle->SetOffset(offset, Position::Begin);
            }
        }
        return false;
    }

    bool BinaryFile::ReadData(void *read_buf, const size_t read_size) {
        if(!this->IsValid()) {
            return false;
        }
        if(!this->CanRead()) {
            return false;
        }
        if(read_size == 0) {
            return true;
        }

        if(this->IsCompressed()) {
            if(!this->ReadDecompressedData(read_buf, read_size)) {
                return false;
            }
        }
        else {
            if(!this->file_handle->Read(read_buf, read_size)) {
                return false;
            }
        }

        return true;
    }

    bool BinaryFile::WriteData(const void *write_buf, const size_t write_size) {
        if(!this->IsValid()) {
            return false;
        }
        if(!this->CanWrite()) {
            return false;
        }
        if(write_size == 0) {
            return true;
        }

        if(this->IsCompressed()) {
            if(!this->WriteDecompressedData(write_buf, write_size)) {
                return false;
            }
        }
        else {
            if(!this->file_handle->Write(write_buf, write_size)) {
                return false;
            }
        }

        return true;
    }

}