
#pragma once
#include <ntr/fs/fs_BinaryFile.hpp>
#include <fstream>

namespace ntr::fs {

    // Note: these are just wrappers for stdio stuff (the rest of this library's fs code is basically nitrofs filesystems)

    struct StdioFileHandle : public FileHandle {
        FILE *file;

        StdioFileHandle() : file(nullptr) {}

        bool Exists(const std::string &path, size_t &out_size) override;
        Result Open(const std::string &path, const OpenMode mode) override;
        Result GetSize(size_t &out_size) override;
        Result SetOffset(const size_t offset, const Position pos) override;
        Result GetOffset(size_t &out_offset) override;
        Result Read(void *read_buf, const size_t read_size, size_t &out_read_size) override;
        Result Write(const void *write_buf, const size_t write_size) override;
        Result Close() override;
    };

    Result CreateStdioDirectory(const std::string &dir);
    bool IsStdioFile(const std::string &path);
    Result GetStdioFileSize(const std::string &path, size_t &out_size);
    Result DeleteStdioFile(const std::string &path);
    Result DeleteStdioDirectory(const std::string &path);
    Result CopyStdioFiles(const std::string &file, const std::string &new_file, const size_t offset);
    Result ListAllStdioFiles(const std::string &path, std::vector<std::string> &out_files);

    inline void EnsureBaseStdioDirectoryExists(const std::string &path) {
        const auto base_dir = GetBaseDirectory(path);
        CreateStdioDirectory(base_dir);
    }

    template<typename T>
    struct ExternalFsFileHandle : public FileHandle {
        static_assert(std::is_base_of_v<ExternalFsFileFormat, T>);

        std::shared_ptr<T> ext_fs_file;
        bool rw_from_ext_fs_file;
        std::string ext_fs_path;
        fs::BinaryFile ext_fs_bin_file;

        ExternalFsFileHandle(std::shared_ptr<T> ext_fs_file) : ext_fs_file(ext_fs_file) {
            // TODO: check err here?
            fs::CreateStdioDirectory(ext_fs_file->ext_fs_root_path);
        }

        virtual bool ExistsImpl(const std::string &path, size_t &out_size) = 0;
        virtual Result OpenImpl(const std::string &path) = 0;
        virtual Result GetSizeImpl(size_t &out_offset) = 0;
        virtual Result SetOffsetImpl(const size_t offset, const fs::Position pos) = 0;
        virtual Result GetOffsetImpl(size_t &out_offset) = 0;
        virtual Result ReadImpl(void *read_buf, const size_t read_size, size_t &out_read_size) = 0;
        virtual Result CloseImpl() = 0;

        bool Exists(const std::string &path, size_t &out_size) override {
            return this->ExistsImpl(path, out_size);
        }

        Result Open(const std::string &path, const fs::OpenMode mode) override {
            this->ext_fs_path = this->ext_fs_file->GetExternalFsPath(path);
    
            this->rw_from_ext_fs_file = fs::IsStdioFile(this->ext_fs_path) || fs::CanWriteWithMode(mode);
            if(this->rw_from_ext_fs_file) {
                EnsureBaseStdioDirectoryExists(this->ext_fs_path);
                return this->ext_fs_bin_file.Open(std::make_shared<fs::StdioFileHandle>(), this->ext_fs_path, mode);
            }
            else {
                return this->OpenImpl(path);
            }
        }

        Result GetSize(size_t &out_size) override {
            if(this->rw_from_ext_fs_file) {
                // ???
                return this->ext_fs_bin_file.GetSize(out_size);
            }
            else {
                return this->GetSizeImpl(out_size);
            }
        }

        Result SetOffset(const size_t offset, const fs::Position pos) override {
            if(this->rw_from_ext_fs_file) {
                switch(pos) {
                    case fs::Position::Begin: {
                        return this->ext_fs_bin_file.SetAbsoluteOffset(offset);
                    }
                    case fs::Position::Current: {
                        return this->ext_fs_bin_file.MoveOffset(offset);
                    }
                    default: {
                        NTR_R_FAIL(ResultInvalidSeekPosition);
                    }
                }
            }
            else {
                return this->SetOffsetImpl(offset, pos);
            }
        }

        Result GetOffset(size_t &out_offset) override {
            if(this->rw_from_ext_fs_file) {
                return this->ext_fs_bin_file.GetAbsoluteOffset(out_offset);
            }
            else {
                return this->GetOffsetImpl(out_offset);
            }
        }

        Result Read(void *read_buf, const size_t read_size, size_t &out_read_size) override {
            if(this->rw_from_ext_fs_file) {
                return this->ext_fs_bin_file.ReadData(read_buf, read_size, out_read_size);
            }
            else {
                return this->ReadImpl(read_buf, read_size, out_read_size);
            }
        }

        Result Write(const void *write_buf, const size_t write_size) override {
            if(this->rw_from_ext_fs_file) {
                return this->ext_fs_bin_file.WriteData(write_buf, write_size);
            }
            else {
                NTR_R_FAIL(ResultWriteNotSupported);
            }
        }

        Result Close() override {
            this->ext_fs_path.clear();
            if(this->rw_from_ext_fs_file) {
                return this->ext_fs_bin_file.Close();
            }
            else {
                return this->CloseImpl();
            }
        }
    };

    class StdioFileSystemIterator {
        private:
            std::vector<dirent> loaded_entries;

        public:
            StdioFileSystemIterator(const std::string &path) {
                
            }
    };

    #define FS_FOR_EACH_STDIO_ENTRY(path, ...) { \
        auto dir = opendir(path.c_str()); \
        if(dir) { \
            while(true) { \
                auto ent = readdir(dir); \
                if(ent == nullptr) { \
                    break; \
                } \
                const std::string entry_name = ent->d_name; \
                const std::string entry_path = path + "/" + entry_name; \
                const bool is_file = ent->d_type & DT_REG; \
                const bool is_dir = ent->d_type & DT_DIR; \
                __VA_ARGS__ \
            } \
            closedir(dir); \
        } \
    }

}