
#pragma once
#include <ntr/fs/fs_Base.hpp>
#include <ntr/util/util_Compression.hpp>
#include <ntr/util/util_Memory.hpp>

namespace ntr::fs {

    class BinaryFile {
        private:
            std::shared_ptr<FileHandle> file_handle;
            bool ok;
            OpenMode mode;
            FileCompression comp;
            u8 *dec_file_data;
            size_t dec_file_offset;
            size_t dec_file_size;
            size_t dec_file_buf_size;

            bool LoadDecompressedData();
            bool SaveCompressedData();
            bool ReadDecompressedData(void *read_buf, const size_t read_size);
            bool WriteDecompressedData(const void *write_buf, const size_t write_size);
            bool ReallocateDecompressedData(const size_t new_size);

        public:
            BinaryFile() : file_handle(), ok(false), mode(OpenMode::Read), comp(FileCompression::None), dec_file_data(nullptr), dec_file_offset(0), dec_file_size(0) {}
            BinaryFile(const BinaryFile&) = delete;

            ~BinaryFile() {
                this->Close();
            }

            bool Open(std::shared_ptr<FileHandle> file_handle, const std::string &path, const OpenMode mode, const FileCompression comp = FileCompression::None);
            bool Close();

            inline bool IsValid() {
                return this->ok && static_cast<bool>(this->file_handle);
            }

            inline bool IsCompressed() {
                return this->comp != FileCompression::None;
            }

            inline constexpr bool CanRead() {
                return CanReadWithMode(this->mode);
            }

            inline constexpr bool CanWrite() {
                return CanWriteWithMode(this->mode);
            }

            inline size_t GetSize() {
                if(!this->IsValid()) {
                    return 0;
                }

                if(this->IsCompressed()) {
                    return this->dec_file_size;
                }
                else {
                    return this->file_handle->GetSize();
                }
            }

            bool CopyFrom(BinaryFile &other_bf, const size_t size);

            bool SetAbsoluteOffset(const size_t offset);

            inline size_t GetAbsoluteOffset() {
                if(!this->IsValid()) {
                    return 0;
                }
                
                if(this->IsCompressed()) {
                    return this->dec_file_offset;
                }
                else {
                    return this->file_handle->GetOffset();
                }
            }

            inline bool SetAtEnd() {
                if(!this->IsValid()) {
                    return false;
                }
                
                if(this->IsCompressed()) {
                    this->dec_file_offset = this->dec_file_size;
                    return true;
                }
                else {
                    return this->file_handle->SetOffset(0, fs::Position::End);
                }
            }

            inline bool MoveOffset(const size_t size) {
                if(this->IsValid()) {
                    if(this->IsCompressed()) {
                        if((this->dec_file_offset + size) > this->dec_file_buf_size) {
                            if(!this->ReallocateDecompressedData(this->dec_file_offset + size)) {
                                return false;
                            }
                        }
                        this->dec_file_offset += size;
                        if(this->dec_file_offset > this->dec_file_size) {
                            this->dec_file_size = this->dec_file_offset;
                        }
                        return true;
                    }
                    else {
                        return this->file_handle->SetOffset(size, Position::Current);
                    }
                }
                return false;
            }

            bool ReadData(void *read_buf, const size_t read_size);

            template<typename T>
            inline bool Read(T &out_t) {
                return this->ReadData(std::addressof(out_t), sizeof(out_t));
            }

            template<typename C>
            inline bool ReadNullTerminatedString(std::basic_string<C> &out_str, const size_t tmp_buf_size = 0x200) {
                // Note: this approach is significantly faster than the classic read-char-by-char approach, specially when reading hundreds of strings ;)
                out_str.clear();
                const auto old_offset = this->GetAbsoluteOffset();
                const auto available_size = this->GetSize() - old_offset;
                if(available_size == 0) {
                    return false;
                }
                const auto r_size = std::min(tmp_buf_size, available_size);
                auto buf = util::NewArray<C>(r_size);
                if(!this->ReadData(buf, r_size)) {
                    delete[] buf;
                    return false;
                }
                for(size_t i = 0; i < r_size; i++) {
                    if(buf[i] == static_cast<C>(0)) {
                        this->SetAbsoluteOffset(old_offset + i + 1);
                        out_str = std::basic_string<C>(buf, i);
                        delete[] buf;
                        return true;
                    }
                }
                
                this->SetAbsoluteOffset(old_offset);
                delete[] buf;
                return ReadNullTerminatedString(out_str, tmp_buf_size * 2);
            }

            bool WriteData(const void *write_buf, const size_t write_size);

            template<typename C>
            inline bool WriteString(const std::basic_string<C> &str) {
                return this->WriteData(str.c_str(), str.length() * sizeof(C));
            }

            inline bool WriteCString(const char *str) {
                const auto str_len = std::strlen(str);
                return this->WriteData(str, str_len);
            }

            template<typename T>
            inline bool Write(const T t) {
                return this->WriteData(std::addressof(t), sizeof(t));
            }

            template<typename T>
            inline bool WriteVector(const std::vector<T> &vec) {
                return this->WriteData(vec.data(), vec.size() * sizeof(T));
            }

            template<typename C>
            inline bool WriteNullTerminatedString(const std::basic_string<C> &str) {
                if(!this->WriteString(str)) {
                    return false;
                }
                if(!this->Write(static_cast<C>(0))) {
                    return false;
                }
                return true;
            }
    };

}