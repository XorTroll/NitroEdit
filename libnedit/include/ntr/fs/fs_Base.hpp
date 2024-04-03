
#pragma once
#include <ntr/ntr_Include.hpp>

namespace ntr::fs {

    constexpr size_t CopyBufferSize = 0x10000;
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
        Current
    };

    enum class FileCompression : u8 {
        None,
        LZ77
    };

    struct FileHandle {
        virtual bool Exists(const std::string &path, size_t &out_size) = 0;
        virtual Result Open(const std::string &path, const OpenMode mode) = 0;
        virtual Result GetSize(size_t &out_size) = 0;
        virtual Result SetOffset(const size_t offset, const Position pos) = 0;
        virtual Result GetOffset(size_t &out_offset) = 0;
        virtual Result Read(void *read_buf, const size_t read_size, size_t &out_read_size) = 0;
        virtual Result Write(const void *write_buf, const size_t write_size) = 0;
        virtual Result Close() = 0;
    };

    struct FileFormat {
        std::string read_path;
        std::shared_ptr<fs::FileHandle> read_file_handle;
        std::string write_path;
        std::shared_ptr<fs::FileHandle> write_file_handle;
        fs::FileCompression comp;

        virtual Result ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) = 0;

        inline Result Validate(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp = fs::FileCompression::None) {
            return this->ValidateImpl(path, file_handle, comp);
        }

        virtual Result ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) = 0;

        inline Result ReadFrom(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp = fs::FileCompression::None) {
            NTR_R_TRY(this->ValidateImpl(path, file_handle, comp));
            NTR_R_TRY(this->ReadImpl(path, file_handle, comp));

            this->read_path = path;
            this->read_file_handle = file_handle;
            this->comp = comp;
            NTR_R_SUCCEED();
        }

        inline Result ReadCompressedFrom(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle) {
            return this->ReadFrom(path, file_handle, fs::FileCompression::LZ77);
        }

        virtual Result WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) = 0;

        inline Result WriteTo(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle) {
            NTR_R_TRY(this->WriteImpl(path, file_handle, this->comp));

            this->write_path = path;
            this->write_file_handle = file_handle;
            NTR_R_SUCCEED();
        }

        inline Result WriteTo(std::shared_ptr<fs::FileHandle> file_handle) {
            return this->WriteTo(this->read_path, file_handle);
        }
    };

    void SetExternalFsDirectory(const std::string &path);
    std::string &GetExternalFsDirectory();

    struct ExternalFsFileFormat : public FileFormat {
        u32 ext_fs_id;
        std::string ext_fs_root_path;

        ExternalFsFileFormat();
        
        inline std::string GetExternalFsPath(const std::string &path) {
            return this->ext_fs_root_path + "/" + path;
        }

        inline std::string GetBasePath(const std::string &ext_fs_path) {
            return ext_fs_path.substr(this->ext_fs_root_path.length() + 1);
        }

        Result WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override {
            NTR_R_SUCCEED();
        }

        virtual Result SaveFileSystem() = 0;
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