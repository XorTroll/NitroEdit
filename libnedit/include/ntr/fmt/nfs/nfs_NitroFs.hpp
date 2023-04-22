
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

        std::string GetName(fs::BinaryFile &base_bf) const;
    };

    struct NitroFile : public NitroEntryBase {
        size_t offset;
        size_t size;

        bool Read(fs::BinaryFile &base_bf, const size_t offset, void *read_buf, const size_t read_size) const;
    };

    struct NitroDirectory : public NitroEntryBase {
        bool is_root;
        std::vector<NitroDirectory> dirs;
        std::vector<NitroFile> files;
    };

    bool ReadNitroFsFrom(const size_t fat_data_offset, const size_t fnt_data_offset, fs::BinaryFile &bf, NitroDirectory &out_fs_root_dir);

    struct NitroFsFileFormat : public fs::ExternalFsFileFormat {
        nfs::NitroDirectory nitro_fs;

        NitroFsFileFormat() {}

        virtual size_t GetBaseOffset() {
            return 0;
        }

        virtual bool HasAlignmentBetweenFileData() {
            return false;
        }

        virtual size_t GetFatEntriesOffset() = 0;
        virtual size_t GetFatEntryCount() = 0;

        virtual bool OnFileSystemWrite(fs::BinaryFile &w_bf, const ssize_t size_diff) = 0;

        bool LookupFile(const std::string &path, NitroFile &out_file) const;
        std::string GetName(const NitroEntryBase &entry) const;
        
        inline bool DoWithReadFile(std::function<void(fs::BinaryFile&)> fn) const {
            fs::BinaryFile bf;
            if(!bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp)) {
                return false;
            }

            fn(bf);
            return true;
        }
        
        bool SaveFileSystem() override;
    };

    template<typename T>
    struct NitroFsFileHandle : public fs::ExternalFsFileHandle<T> {
        static_assert(std::is_base_of_v<NitroFsFileFormat, T>);

        NitroFile file;
        fs::BinaryFile base_bf;

        NitroFsFileHandle(T &nitro_fs_file) : fs::ExternalFsFileHandle<T>(nitro_fs_file) {}

        bool ExistsImpl(const std::string &path, size_t &out_size) override {
            NitroFile file = {};
            if(this->ext_fs_file.LookupFile(path, file)) {
                out_size = file.size;
                return true;
            }
            return false;
        }

        bool OpenImpl(const std::string &path) override {
            if(!this->ext_fs_file.LookupFile(path, this->file)) {
                return false;
            }

            const auto ext = fs::GetFileExtension(path);
            if(!this->base_bf.Open(this->ext_fs_file.read_file_handle, this->ext_fs_file.read_path, fs::OpenMode::Read, this->ext_fs_file.comp)) {
                return false;
            }
            const auto f_base_offset = this->ext_fs_file.GetBaseOffset() + this->file.offset;
            if(!this->base_bf.SetAbsoluteOffset(f_base_offset)) {
                return false;
            }

            return true;
        }

        size_t GetSizeImpl() override {
            return this->file.size;
        }

        bool SetOffsetImpl(const size_t offset, const fs::Position pos) override {
            const auto f_base_offset = this->ext_fs_file.GetBaseOffset() + this->file.offset;
            const auto f_cur_offset = this->base_bf.GetAbsoluteOffset() - f_base_offset;
            switch(pos) {
                case fs::Position::Begin: {
                    if(offset > this->file.size) {
                        return false;
                    }
                    this->base_bf.SetAbsoluteOffset(f_base_offset + offset);
                    break;
                }
                case fs::Position::Current: {
                    if((f_cur_offset + offset) > this->file.size) {
                        return false;
                    }
                    this->base_bf.SetAbsoluteOffset(f_base_offset + f_cur_offset + offset);
                    break;
                }
                case fs::Position::End: {
                    if(offset != 0) {
                        return false;
                    }
                    this->base_bf.SetAbsoluteOffset(f_base_offset + this->file.size);
                    break;
                }
            }
            return true;
        }

        size_t GetOffsetImpl() override {
            const auto f_base_offset = this->ext_fs_file.GetBaseOffset() + this->file.offset;
            return this->base_bf.GetAbsoluteOffset() - f_base_offset;
        }

        bool ReadImpl(void *read_buf, const size_t read_size) override {
            if((this->GetOffsetImpl() + read_size) > this->file.size) {
                return false;
            }
            return this->base_bf.ReadData(read_buf, read_size);
        }

        bool CloseImpl() override {
            return this->base_bf.Close();
        }
    };

}