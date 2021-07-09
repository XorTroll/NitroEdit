
#pragma once
#include <base_Include.hpp>

namespace fs {

    constexpr size_t CopyBufferSize = 0x100000;
    constexpr size_t ReallocBufferSize = 0x10000;

    enum class OpenMode : u8 {
        Read,
        Write,
        Update
    };

    inline constexpr bool CanReadWithMode(const OpenMode mode) {
        return mode == OpenMode::Read;
    }

    inline constexpr bool CanWriteWithMode(const OpenMode mode) {
        return (mode == OpenMode::Write) || (mode == OpenMode::Update);
    }

    enum class Position : u8 {
        Begin,
        Current,
        End
    };

    enum class FileCompression : u8 {
        None,
        LZ77
    };

    struct FileHandle {
        virtual bool Exists(const std::string &path, size_t &out_size) = 0;
        virtual bool Open(const std::string &path, const OpenMode mode) = 0;
        virtual size_t GetSize() = 0;
        virtual bool SetOffset(const size_t offset, const Position pos) = 0;
        virtual size_t GetOffset() = 0;
        virtual bool Read(void *read_buf, const size_t read_size) = 0;
        virtual bool Write(const void *write_buf, const size_t write_size) = 0;
        virtual bool Close() = 0;
    };

    struct FileFormat {
        std::string read_path;
        std::shared_ptr<fs::FileHandle> read_file_handle;
        std::string write_path;
        std::shared_ptr<fs::FileHandle> write_file_handle;
        fs::FileCompression comp;

        virtual bool ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) = 0;

        inline bool ReadFrom(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp = fs::FileCompression::None) {
            if(this->ReadImpl(path, file_handle, comp)) {
                this->read_path = path;
                this->read_file_handle = file_handle;
                this->comp = comp;
                return true;
            }

            return false;
        }

        inline bool ReadCompressedFrom(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle) {
            return this->ReadFrom(path, file_handle, fs::FileCompression::LZ77);
        }

        virtual bool WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) = 0;

        inline bool WriteTo(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle) {
            if(this->WriteImpl(path, file_handle, this->comp)) {
                this->write_path = path;
                this->write_file_handle = file_handle;
                return true;
            }

            return false;
        }

        inline bool WriteTo(std::shared_ptr<fs::FileHandle> file_handle) {
            return this->WriteTo(this->read_path, file_handle);
        }
    };

    struct ExternalFsFileFormat : public FileFormat {
        u32 ext_fs_id;
        std::string ext_fs_root_path;

        ExternalFsFileFormat() : ext_fs_id(static_cast<u32>(rand())) {
            this->ext_fs_root_path = ExternalFsDirectory + "/" + std::to_string(this->ext_fs_id);
        }
        
        inline std::string GetExternalFsPath(const std::string &path) {
            return this->ext_fs_root_path + "/" + path;
        }

        inline std::string GetBasePath(const std::string &ext_fs_path) {
            return ext_fs_path.substr(this->ext_fs_root_path.length() + 1);
        }

        bool WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override {
            return true;
        }

        virtual bool SaveFileSystem() = 0;
    };

    std::string GetAbsolutePath(const std::string &path);

    inline std::string RemoveLastEntry(const std::string &path) {
        const auto find_pos = path.find_last_of("/");
        if(find_pos == std::string::npos) {
            return "";
        }
        else {
            return path.substr(0, find_pos);
        }
    }

    inline std::string GetFileName(const std::string &path) {
        return path.substr(path.find_last_of("/") + 1);
    }

    inline std::string GetBaseDirectory(const std::string &path) {
        return RemoveLastEntry(path);
    }

    inline std::string GetFileExtension(const std::string &path) {
        return path.substr(path.find_last_of(".") + 1);
    }

}