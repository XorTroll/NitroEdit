
#pragma once
#include <ntr/fs/fs_BinaryFile.hpp>
#include <fstream>

namespace ntr::fs {

    // Note: these are just wrappers for stdio stuff (the rest of this library's fs code is basically nitrofs filesystems)

    struct StdioFileHandle : public FileHandle {
        FILE *file;

        StdioFileHandle() : file(nullptr) {}

        bool Exists(const std::string &path, size_t &out_size) override;
        bool Open(const std::string &path, const OpenMode mode) override;
        size_t GetSize() override;
        bool SetOffset(const size_t offset, const Position pos) override;
        size_t GetOffset() override;
        bool Read(void *read_buf, const size_t read_size) override;
        bool Write(const void *write_buf, const size_t write_size) override;
        bool Close() override;
    };

    bool CreateStdioDirectory(const std::string &dir);
    bool IsStdioFile(const std::string &path);
    size_t GetStdioFileSize(const std::string &path);
    bool DeleteStdioFile(const std::string &path);
    bool DeleteStdioDirectory(const std::string &path);
    bool CopyStdioFiles(const std::string &file, const std::string &new_file, const size_t offset);
    bool AppendStdioFiles(const std::string &file, const std::string &ap_file);
    bool ListAllStdioFiles(const std::string &path, std::vector<std::string> &out_files);

    inline void EnsureBaseStdioDirectoryExists(const std::string &path) {
        const auto base_dir = GetBaseDirectory(path);
        CreateStdioDirectory(base_dir);
    }

    template<typename T>
    struct ExternalFsFileHandle : public FileHandle {
        static_assert(std::is_base_of_v<ExternalFsFileFormat, T>);

        T &ext_fs_file;
        bool rw_from_ext_fs_file;
        std::string ext_fs_path;
        fs::BinaryFile ext_fs_bin_file;

        ExternalFsFileHandle(T &ext_fs_file) : ext_fs_file(ext_fs_file) {
            fs::CreateStdioDirectory(ext_fs_file.ext_fs_root_path);
        }

        virtual bool ExistsImpl(const std::string &path, size_t &out_size) = 0;
        virtual bool OpenImpl(const std::string &path) = 0;
        virtual size_t GetSizeImpl() = 0;
        virtual bool SetOffsetImpl(const size_t offset, const fs::Position pos) = 0;
        virtual size_t GetOffsetImpl() = 0;
        virtual bool ReadImpl(void *read_buf, const size_t read_size) = 0;
        virtual bool CloseImpl() = 0;

        bool Exists(const std::string &path, size_t &out_size) override {
            return this->ExistsImpl(path, out_size);
        }

        bool Open(const std::string &path, const fs::OpenMode mode) override {
            this->ext_fs_path = this->ext_fs_file.GetExternalFsPath(path);
            this->rw_from_ext_fs_file = fs::IsStdioFile(this->ext_fs_path) || fs::CanWriteWithMode(mode);
            if(this->rw_from_ext_fs_file) {
                EnsureBaseStdioDirectoryExists(this->ext_fs_path);
                if(!this->ext_fs_bin_file.Open(std::make_shared<fs::StdioFileHandle>(), this->ext_fs_path, mode)) {
                    return false;
                }
            }
            else {
                if(!this->OpenImpl(path)) {
                    return false;
                }
            }

            return true;
        }

        size_t GetSize() override {
            if(this->rw_from_ext_fs_file) {
                return this->ext_fs_bin_file.GetAbsoluteOffset();
            }
            else {
                return this->GetSizeImpl();
            }
        }

        bool SetOffset(const size_t offset, const fs::Position pos) override {
            if(this->rw_from_ext_fs_file) {
                switch(pos) {
                    case fs::Position::Begin: {
                        return this->ext_fs_bin_file.SetAbsoluteOffset(offset);
                    }
                    case fs::Position::Current: {
                        return this->ext_fs_bin_file.MoveOffset(offset);
                    }
                    case fs::Position::End: {
                        if(offset != 0) {
                            return false;
                        }
                        return this->ext_fs_bin_file.SetAtEnd();
                    }
                }
            }
            else {
                return this->SetOffsetImpl(offset, pos);
            }
            return true;
        }

        size_t GetOffset() override {
            if(this->rw_from_ext_fs_file) {
                return this->ext_fs_bin_file.GetAbsoluteOffset();
            }
            else {
                return this->GetOffsetImpl();
            }
        }

        bool Read(void *read_buf, const size_t read_size) override {
            if(this->rw_from_ext_fs_file) {
                return this->ext_fs_bin_file.ReadData(read_buf, read_size);
            }
            else {
                return this->ReadImpl(read_buf, read_size);
            }
        }

        bool Write(const void *write_buf, const size_t write_size) override {
            if(this->rw_from_ext_fs_file) {
                return this->ext_fs_bin_file.WriteData(write_buf, write_size);
            }
            else {
                return false;
            }
        }

        bool Close() override {
            this->ext_fs_path.clear();
            if(this->rw_from_ext_fs_file) {
                return this->ext_fs_bin_file.Close();
            }
            else {
                return this->CloseImpl();
            }
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