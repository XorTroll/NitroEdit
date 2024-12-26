#include <ntr/fs/fs_Stdio.hpp>
#include <unistd.h>

namespace ntr::fs {

    namespace {

        inline constexpr const char *GetOpenMode(const OpenMode mode) {
            switch(mode) {
                case OpenMode::Read: {
                    return "rb";
                }
                case OpenMode::Write: {
                    return "wb";
                }
                case OpenMode::Update: {
                    return "ab";
                }
                default: {
                    return nullptr;
                }
            }
        }

    }

    bool StdioFileHandle::Exists(const std::string &path, size_t &out_size) {
        struct stat st;
        if(stat(path.c_str(), &st) == 0) {
            if(st.st_mode & S_IFREG) {
                out_size = st.st_size;
                return true;
            }
        }
        return false;
    }

    Result StdioFileHandle::Open(const std::string &path, const OpenMode mode) {
        const auto f_mode = GetOpenMode(mode);
        if(f_mode == nullptr) {
            NTR_R_FAIL(ResultInvalidFileOpenMode);
        }

        this->file = fopen(path.c_str(), f_mode);
        if(this->file == nullptr) {
            NTR_R_FAIL(ResultUnableToOpenStdioFile);
        }
        else {
            NTR_R_SUCCEED();
        }
    }

    Result StdioFileHandle::GetSize(size_t &out_size) {
        const auto cur_pos = ftell(this->file);
        if(fseek(this->file, 0, SEEK_END) != 0) {
            NTR_R_FAIL(ResultUnableToSeekStdioFile);
        }
        const auto f_size = ftell(this->file);
        if(fseek(this->file, cur_pos, SEEK_SET) != 0) {
            NTR_R_FAIL(ResultUnableToSeekStdioFile);
        }

        out_size = f_size;
        NTR_R_SUCCEED();
    }

    Result StdioFileHandle::SetOffset(const size_t offset, const Position pos) {
        int whence;
        switch(pos) {
            case Position::Begin: {
                whence = SEEK_SET;
                break;
            }
            case Position::Current: {
                whence = SEEK_CUR;
                break;
            }
            default: {
                NTR_R_FAIL(ResultInvalidSeekPosition);
            }
        }

        if(fseek(this->file, offset, whence) != 0) {
            NTR_R_FAIL(ResultUnableToSeekStdioFile);
        }
        else {
            NTR_R_SUCCEED();
        }
    }

    Result StdioFileHandle::GetOffset(size_t &out_offset) {
        out_offset = ftell(this->file);
        NTR_R_SUCCEED();
    }

    Result StdioFileHandle::Read(void *read_buf, const size_t read_size, size_t &out_read_size) {
        const auto got_read_size = fread(read_buf, 1, read_size, this->file);
        if(got_read_size == 0) {
            NTR_R_FAIL(ResultUnableToReadStdioFile);
        }
        else {
            out_read_size = got_read_size;
            NTR_R_SUCCEED();
        }
    }

    Result StdioFileHandle::Write(const void *write_buf, const size_t write_size) {
        if(fwrite(write_buf, write_size, 1, this->file) != 1) {
            NTR_R_FAIL(ResultUnableToWriteStdioFile);
        }
        else {
            NTR_R_SUCCEED();
        }
    }

    Result StdioFileHandle::Close() {
        if(this->file == nullptr) {
            NTR_R_FAIL(ResultInvalidFile);
        }
        
        auto cur_file = this->file;
        this->file = nullptr;
        if(fclose(cur_file) != 0) {
            NTR_R_FAIL(ResultUnableToCloseStdioFile);
        }
        else {
            NTR_R_SUCCEED();
        }
    }

    Result CreateStdioDirectory(const std::string &dir) {
        auto pos_init = 0;
        auto pos_found = 0;

        std::string token = "";
        std::string cur_dir = "";
        while(pos_found >= 0) {
            pos_found = dir.find('/', pos_init);
            token = dir.substr(pos_init, pos_found - pos_init);

            pos_init = pos_found + 1;

            if(!cur_dir.empty()) {
                cur_dir += "/";
            }
            cur_dir += token;
            if(mkdir(cur_dir.c_str(), 777) != 0) {
                if(errno != EEXIST) {
                    NTR_R_FAIL(ResultUnableToCreateStdioDirectory);
                }
            }
        }
        NTR_R_SUCCEED();
    }

    bool IsStdioFile(const std::string &path) {
        struct stat st;
        if(stat(path.c_str(), &st) == 0) {
            if(st.st_mode & S_IFREG) {
                return true;
            }
        }
        return false;
    }

    Result GetStdioFileSize(const std::string &path, size_t &out_size) {
        struct stat st;
        if(stat(path.c_str(), &st) == 0) {
            if(st.st_mode & S_IFREG) {
                out_size = st.st_size;
                NTR_R_SUCCEED();
            }
        }

        NTR_R_FAIL(ResultUnableToOpenStdioFile);
    }

    Result DeleteStdioFile(const std::string &path) {
        if(remove(path.c_str()) != 0) {
            NTR_R_FAIL(ResultUnableToDeleteStdioFile);
        }
        else {
            NTR_R_SUCCEED();
        }
    }

    Result DeleteStdioDirectory(const std::string &path) {
        auto dir = opendir(path.c_str());
        if(dir) {
            std::vector<std::string> subfiles;
            std::vector<std::string> subdirs;
            while(true) {
                const auto ent = readdir(dir);
                if(ent == nullptr) {
                    break;
                }
                if(strcmp(ent->d_name, ".") == 0) {
                    continue;
                }
                if(strcmp(ent->d_name, "..") == 0) {
                    continue;
                }
                const auto subpath = path + "/" + ent->d_name;
                if(ent->d_type & DT_DIR) {
                    subdirs.push_back(subpath);
                }
                else if(ent->d_type & DT_REG) {
                    subfiles.push_back(subpath);
                }
            }
            closedir(dir);
            for(const auto &subfile : subfiles) {
                NTR_R_TRY(DeleteStdioFile(subfile));
            }
            for(const auto &subdir : subdirs) {
                NTR_R_TRY(DeleteStdioDirectory(subdir));
            }
        }

        // Note: intentionally not checking this result since it will fail with NDS/libfat
        /* int res = */ rmdir(path.c_str());
        NTR_R_SUCCEED();
    }

    Result ListAllStdioFiles(const std::string &path, std::vector<std::string> &out_files) {
        auto dir = opendir(path.c_str());
        if(dir) {
            std::vector<std::string> subdirs;
            while(true) {
                const auto ent = readdir(dir);
                if(ent == nullptr) {
                    break;
                }
                if(strcmp(ent->d_name, ".") == 0) {
                    continue;
                }
                if(strcmp(ent->d_name, "..") == 0) {
                    continue;
                }
                const auto subpath = path + "/" + ent->d_name;
                if(ent->d_type & DT_DIR) {
                    subdirs.push_back(subpath);
                }
                else if(ent->d_type & DT_REG) {
                    out_files.push_back(subpath);
                }
            }
            closedir(dir);
            for(const auto &subdir : subdirs) {
                NTR_R_TRY(ListAllStdioFiles(subdir, out_files));
            }
        }
        NTR_R_SUCCEED();
    }

}