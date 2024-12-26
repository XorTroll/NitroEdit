
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
            util::LzVersion comp_lz_ver;
            u8 *dec_file_data;
            size_t dec_file_offset;
            size_t dec_file_size;
            size_t dec_file_buf_size;

            Result LoadDecompressedData();
            Result SaveCompressedData();
            Result ReadDecompressedData(void *read_buf, const size_t read_size, size_t &out_read_size);
            Result WriteDecompressedData(const void *write_buf, const size_t write_size);
            Result ReallocateDecompressedData(const size_t new_size);

        public:
            BinaryFile() : file_handle(), ok(false), mode(OpenMode::Read), comp(FileCompression::None), comp_lz_ver(util::LzVersion::LZ10), dec_file_data(nullptr), dec_file_offset(0), dec_file_size(0) {}
            BinaryFile(const BinaryFile&) = delete;

            ~BinaryFile() {
                this->Close();
            }

            Result Open(std::shared_ptr<FileHandle> file_handle, const std::string &path, const OpenMode mode, const FileCompression comp = FileCompression::None);
            Result Close();

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

            inline Result GetSize(size_t &out_size) {
                if(!this->IsValid()) {
                    NTR_R_FAIL(ResultInvalidFile);
                }

                if(this->IsCompressed()) {
                    out_size = this->dec_file_size;
                    NTR_R_SUCCEED();
                }
                else {
                    return this->file_handle->GetSize(out_size);
                }
            }

            Result CopyFrom(BinaryFile &other_bf, const size_t size);

            Result SetAbsoluteOffset(const size_t offset);

            inline Result GetAbsoluteOffset(size_t &out_offset) {
                if(!this->IsValid()) {
                    NTR_R_FAIL(ResultInvalidFile);
                }
                
                if(this->IsCompressed()) {
                    out_offset = this->dec_file_offset;
                    NTR_R_SUCCEED();
                }
                else {
                    return this->file_handle->GetOffset(out_offset);
                }
            }

            inline Result SetAtEnd() {
                if(!this->IsValid()) {
                    NTR_R_FAIL(ResultInvalidFile);
                }
                
                if(this->IsCompressed()) {
                    this->dec_file_offset = this->dec_file_size;
                    NTR_R_SUCCEED();
                }
                else {
                    size_t f_size;
                    NTR_R_TRY(this->GetSize(f_size));
                    return this->file_handle->SetOffset(f_size, Position::Begin);
                }
            }

            inline Result MoveOffset(const size_t size) {
                if(!this->IsValid()) {
                    NTR_R_FAIL(ResultInvalidFile);
                }

                if(this->IsCompressed()) {
                    if((this->dec_file_offset + size) > this->dec_file_buf_size) {
                        NTR_R_TRY(this->ReallocateDecompressedData(this->dec_file_offset + size));
                    }
                    this->dec_file_offset += size;
                    if(this->dec_file_offset > this->dec_file_size) {
                        this->dec_file_size = this->dec_file_offset;
                    }
                    NTR_R_SUCCEED();
                }
                else {
                    return this->file_handle->SetOffset(size, Position::Current);
                }
            }

            Result ReadData(void *read_buf, const size_t read_size, size_t &out_read_size);

            inline Result ReadDataExact(void *read_buf, const size_t read_size) {
                size_t actual_read_size;
                NTR_R_TRY(this->ReadData(read_buf, read_size, actual_read_size));

                if(actual_read_size == read_size) {
                    NTR_R_SUCCEED();
                }
                else {
                    NTR_R_FAIL(ResultUnexpectedReadSize);
                }
            }

            template<typename T>
            inline Result Read(T &out_t) {
                return this->ReadDataExact(std::addressof(out_t), sizeof(out_t));
            }

            template<typename T>
            inline Result ReadLEB128(T &out_t) {
                out_t = {};
                while(true) {
                    u8 v;
                    NTR_R_TRY(this->Read(v));

                    out_t = (out_t << 7) | (v & 0x7f);
                    if(!(v & 0x80)) {
                        break;
                    }
                }

                NTR_R_SUCCEED();
            }

            template<typename C>
            inline Result ReadNullTerminatedString(std::basic_string<C> &out_str, const size_t tmp_buf_size = 0x200) {
                // Note: this approach is significantly faster than the classic read-char-by-char approach, specially when reading hundreds of strings ;)
                out_str.clear();
                size_t old_offset;
                NTR_R_TRY(this->GetAbsoluteOffset(old_offset));
                size_t f_size;
                NTR_R_TRY(this->GetSize(f_size));
                const auto available_size = f_size - old_offset;
                if(available_size == 0) {
                    NTR_R_FAIL(ResultEndOfData);
                }
                const auto r_size = std::min(tmp_buf_size, available_size);
                auto buf = util::NewArray<C>(r_size);
                ScopeGuard on_exit_cleanup([&]() {
                    delete[] buf;
                });

                size_t read_size;
                NTR_R_TRY(this->ReadData(buf, r_size, read_size));

                for(size_t i = 0; i < read_size; i++) {
                    if(buf[i] == static_cast<C>(0)) {
                        NTR_R_TRY(this->SetAbsoluteOffset(old_offset + i + 1));
                        out_str = std::basic_string<C>(buf, i);
                        NTR_R_SUCCEED();
                    }
                }
                
                NTR_R_TRY(this->SetAbsoluteOffset(old_offset));
                return ReadNullTerminatedString(out_str, tmp_buf_size * 2);
            }

            Result WriteData(const void *write_buf, const size_t write_size);

            template<typename C>
            inline Result WriteString(const std::basic_string<C> &str) {
                return this->WriteData(str.c_str(), str.length() * sizeof(C));
            }

            inline Result WriteCString(const char *str) {
                const auto str_len = std::strlen(str);
                return this->WriteData(str, str_len);
            }

            template<typename T>
            inline Result Write(const T t) {
                return this->WriteData(std::addressof(t), sizeof(t));
            }

            inline Result WriteEnsureAlignment(const size_t align, size_t &out_pad_size) {
                size_t cur_offset;
                NTR_R_TRY(this->GetAbsoluteOffset(cur_offset));
                out_pad_size = util::AlignUp(cur_offset, align) - cur_offset;

                for(size_t i = 0; i < out_pad_size; i++) {
                    NTR_R_TRY(this->Write<u8>(0));
                }
                NTR_R_SUCCEED();
            }

            template<typename T>
            inline Result WriteVector(const std::vector<T> &vec) {
                return this->WriteData(vec.data(), vec.size() * sizeof(T));
            }

            template<typename C>
            inline Result WriteNullTerminatedString(const std::basic_string<C> &str) {
                NTR_R_TRY(this->WriteString(str));
                NTR_R_TRY(this->Write(static_cast<C>(0)));

                NTR_R_SUCCEED();
            }
    };

}