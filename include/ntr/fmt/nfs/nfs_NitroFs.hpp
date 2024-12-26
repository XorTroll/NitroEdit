
#pragma once
#include <ntr/fs/fs_Stdio.hpp>

namespace ntr::fmt::nfs {

    constexpr u16 InitialDirectoryId = 61440;
    constexpr size_t MaxEntryNameLength = 128;
    constexpr const char RootDirectoryPseudoName[] = "<root>";

    struct DirectoryNameTableEntry {
        u32 start;
        u16 id;
        u16 parent_id;
    };

    struct FileAllocationTableEntry {
        u32 file_start;
        u32 file_end;
    };

    struct NitroEntryBase {
        u32 entry_offset;

        Result GetName(fs::BinaryFile &base_bf, std::string &out_name) const;
    };

    struct NitroFile : public NitroEntryBase {
        size_t offset;
        size_t size;

        Result Read(fs::BinaryFile &base_bf, const size_t offset, void *read_buf, const size_t read_size, size_t &out_read_size) const;
    };

    struct NitroDirectory : public NitroEntryBase {
        bool is_root;
        std::vector<NitroDirectory> dirs;
        std::vector<NitroFile> files;
    };

    Result ReadNitroFsFrom(const size_t fat_data_offset, const size_t fnt_data_offset, fs::BinaryFile &bf, NitroDirectory &out_fs_root_dir);

    struct NitroFsFileFormat : public fs::ExternalFsFileFormat {
        nfs::NitroDirectory nitro_fs;

        NitroFsFileFormat() {}

        virtual size_t GetBaseOffset() {
            return 0;
        }

        virtual bool GetAlignmentBetweenFileData(size_t &out_align) {
            return false;
        }

        virtual size_t GetFatEntriesOffset() const = 0;
        virtual size_t GetFatEntryCount() const = 0;

        virtual Result OnFileSystemWrite(fs::BinaryFile &w_bf, const ssize_t size_diff) = 0;

        Result LookupFile(const std::string &path, NitroFile &out_file) const;
        Result GetName(const NitroEntryBase &entry, std::string &out_name) const;
        
        inline Result DoWithReadFile(std::function<Result(fs::BinaryFile&)> fn) const {
            fs::BinaryFile bf;
            NTR_R_TRY(bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp));

            NTR_R_TRY(fn(bf));
            NTR_R_SUCCEED();
        }
        
        Result SaveFileSystem() override;
    };

    template<typename T>
    struct NitroFsFileHandle : public fs::ExternalFsFileHandle<T> {
        static_assert(std::is_base_of_v<NitroFsFileFormat, T>);

        NitroFile file;
        fs::BinaryFile base_bf;

        NitroFsFileHandle(std::shared_ptr<T> nitro_fs_file) : fs::ExternalFsFileHandle<T>(nitro_fs_file) {}

        bool ExistsImpl(const std::string &path, size_t &out_size) override {
            NitroFile file = {};
            if(this->ext_fs_file->LookupFile(path, file).IsSuccess()) {
                out_size = file.size;
                return true;
            }
            else {
                return false;
            }
        }

        Result OpenImpl(const std::string &path) override {
            NTR_R_TRY(this->ext_fs_file->LookupFile(path, this->file));

            NTR_R_TRY(this->base_bf.Open(this->ext_fs_file->read_file_handle, this->ext_fs_file->read_path, fs::OpenMode::Read, this->ext_fs_file->comp));
            const auto f_base_offset = this->ext_fs_file->GetBaseOffset() + this->file.offset;
            NTR_R_TRY(this->base_bf.SetAbsoluteOffset(f_base_offset));

            NTR_R_SUCCEED();
        }

        Result GetSizeImpl(size_t &out_size) override {
            out_size = this->file.size;
            NTR_R_SUCCEED();
        }

        Result SetOffsetImpl(const size_t offset, const fs::Position pos) override {
            const auto f_base_offset = this->ext_fs_file->GetBaseOffset() + this->file.offset;
            
            size_t base_bf_abs_offset;
            NTR_R_TRY(this->base_bf.GetAbsoluteOffset(base_bf_abs_offset));
            const auto f_cur_offset = base_bf_abs_offset - f_base_offset;
            
            switch(pos) {
                case fs::Position::Begin: {
                    if(offset > this->file.size) {
                        NTR_R_FAIL(ResultEndOfData);
                    }
                    else {
                        return this->base_bf.SetAbsoluteOffset(f_base_offset + offset);
                    }
                }
                case fs::Position::Current: {
                    if((f_cur_offset + offset) > this->file.size) {
                        NTR_R_FAIL(ResultEndOfData);
                    }
                    else {
                        return this->base_bf.SetAbsoluteOffset(f_base_offset + f_cur_offset + offset);
                    }
                }
                default: {
                    NTR_R_FAIL(ResultInvalidSeekPosition);
                }
            }
        }

        Result GetOffsetImpl(size_t &out_offset) override {
            const auto f_base_offset = this->ext_fs_file->GetBaseOffset() + this->file.offset;
            
            size_t base_bf_abs_offset;
            NTR_R_TRY(this->base_bf.GetAbsoluteOffset(base_bf_abs_offset));
            out_offset = base_bf_abs_offset - f_base_offset;
            NTR_R_SUCCEED();
        }

        Result ReadImpl(void *read_buf, const size_t read_size, size_t &out_read_size) override {
            auto actual_read_size = read_size;
            size_t offset;
            NTR_R_TRY(this->GetOffsetImpl(offset));
            if((offset + actual_read_size) > this->file.size) {
                actual_read_size = this->file.size - offset;
            }

            if(actual_read_size > 0) {
                return this->base_bf.ReadData(read_buf, actual_read_size, out_read_size);
            }
            else {
                NTR_R_FAIL(ResultEndOfData);
            }
        }

        Result CloseImpl() override {
            return this->base_bf.Close();
        }
    };

}