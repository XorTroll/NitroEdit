#include <ntr/fmt/fmt_ROM.hpp>
#include <ntr/fmt/fmt_NARC.hpp>
#include <iostream>
#include <cerrno>
#include <unordered_map>

#define FUSE_USE_VERSION 25
#include <fuse.h>

namespace {

    bool g_IsROM = false;
    std::shared_ptr<ntr::fmt::nfs::NitroFsFileFormat> g_NitroFsFile;
    std::unordered_map<size_t, std::pair<std::string, struct stat>> g_NameMap;

    std::vector<std::string> SplitPath(const char *path) {
        auto items = ntr::util::SplitString(path, '/');

        // Root will be an empty list
        for(ssize_t i = 0; i < items.size(); i++) {
            if(items.at(i).empty()) {
                items.erase(items.begin() + i);
                i -= 1;
            }
        }
        
        return items;
    }

    ntr::fmt::nfs::NitroDirectory *LookupParent(const std::vector<std::string> &path_items) {
        // Finds (tries to find) the parent directory of the supplied path (splitted in items already)

        auto cur_dir = &g_NitroFsFile->nitro_fs;
        if(path_items.size() == 1) {
            return cur_dir;
        }

        auto items_copy = path_items;

        g_NitroFsFile->DoWithReadFile([&](ntr::fs::BinaryFile &bf) {
            while(true) {
                auto found = false;
                for(auto &dir: cur_dir->dirs) {
                    const auto dir_name = dir.GetName(bf);
                    if(dir_name == items_copy.front()) {
                        cur_dir = &dir;
                        items_copy.erase(items_copy.begin());
                        found = true;
                        if(items_copy.size() == 1) {
                            return;
                        }
                        break;
                    }
                }
                if(!found) {
                    break;
                }
            }
            cur_dir = nullptr;
        });

        return cur_dir;
    }

    ntr::fmt::nfs::NitroDirectory *GetDirectory(ntr::fmt::nfs::NitroDirectory *parent, const std::string &name) {
        ntr::fmt::nfs::NitroDirectory *cur_dir = nullptr;
        g_NitroFsFile->DoWithReadFile([&](ntr::fs::BinaryFile &bf) {
            for(auto &dir: parent->dirs) {
                const auto dir_name = dir.GetName(bf);
                if(dir_name == name) {
                    cur_dir = &dir;
                    break;
                }
            }
        });

        return cur_dir;
    }

    ntr::fmt::nfs::NitroFile *GetFile(ntr::fmt::nfs::NitroDirectory *parent, const std::string &name) {
        ntr::fmt::nfs::NitroFile *cur_file = nullptr;
        g_NitroFsFile->DoWithReadFile([&](ntr::fs::BinaryFile &bf) {
            for(auto &file: parent->files) {
                const auto file_name = file.GetName(bf);
                if(file_name == name) {
                    cur_file = &file;
                    break;
                }
            }
        });

        return cur_file;
    }

    // Let's keep it read-only for now
    constexpr mode_t Permissions = S_IRUSR | S_IRGRP | S_IROTH;

    int g_Fuse_getattr(const char *path, struct stat *st) {
        *st = {};

        const auto path_items = SplitPath(path);
        if(path_items.empty()) {
            st->st_mode = S_IFDIR | Permissions;
            return 0;
        }

        const auto item_name = path_items.back();
        auto parent_dir = LookupParent(path_items);
        if(parent_dir != nullptr) {
            auto file = GetFile(parent_dir, item_name);
            if(file != nullptr) {
                st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
                st->st_size = file->size;
                return 0;
            }

            auto dir = GetDirectory(parent_dir, item_name);
            if(dir != nullptr) {
                st->st_mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH;
                return 0;
            }
        }

        return -ENOENT;
    }

    int g_Fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
        const auto path_items = SplitPath(path);

        ntr::fmt::nfs::NitroDirectory *dir;
        if(path_items.empty()) {
            dir = &g_NitroFsFile->nitro_fs;
        }
        else {
            auto parent_dir = LookupParent(path_items);
            if(parent_dir != nullptr) {
                dir = GetDirectory(parent_dir, path_items.back());
            }
        }

        if(dir != nullptr) {
            g_NitroFsFile->DoWithReadFile([&](ntr::fs::BinaryFile &bf) {
                for(auto &cur_dir: dir->dirs) {
                    const auto dir_name = cur_dir.GetName(bf);
                    struct stat st = {
                        .st_mode = S_IFDIR | Permissions
                    };
                    filler(buf, dir_name.c_str(), &st, 0);
                }
                for(auto &cur_file: dir->files) {
                    const auto file_name = cur_file.GetName(bf);
                    struct stat st = {
                        .st_mode = S_IFREG | Permissions,
                        .st_size = cur_file.size
                    };
                    filler(buf, file_name.c_str(), &st, 0);
                }
            });

            return 0;
        }

        return -ENOENT;
    }

    int g_Fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
        const auto path_items = SplitPath(path);
        const auto item_name = path_items.back();
        auto parent_dir = LookupParent(path_items);
        if(parent_dir != nullptr) {
            auto file = GetFile(parent_dir, item_name);
            if(file != nullptr) {
                auto read_ok = false;
                g_NitroFsFile->DoWithReadFile([&](ntr::fs::BinaryFile &bf) {
                    read_ok = file->Read(bf, offset, buf, size);
                });
                if(read_ok) {
                    return size;
                }
                else {
                    return -EIO;
                }
            }
        }

        return -ENOENT;
    }

    constexpr struct fuse_operations g_FuseOperations = {
        .getattr = g_Fuse_getattr,
        .read = g_Fuse_read,
        .readdir = g_Fuse_readdir
    };

    int RunFuse(std::vector<char*> &args) {
        return fuse_main(args.size(), args.data(), &g_FuseOperations);
    }

}

int main(int argc, char **argv) {
    // Remove argv[1] as it's our input fs, the rest of the args are like fusermount ones
    std::vector<char*> args;
    std::string nitro_path;
    for(int i = 0; i < argc; i++) {
        if(i == 1) {
            nitro_path = argv[i];
        }
        else {
            args.push_back(argv[i]);
        }
    }

    auto rom = std::make_shared<ntr::fmt::ROM>();
    if(rom->ReadFrom(nitro_path, std::make_shared<ntr::fs::StdioFileHandle>())) {
        g_NitroFsFile = rom;
        g_IsROM = true;
        std::cout << "Mounting NDS(i) ROM: '" << nitro_path << "'..." << std::endl;
        return RunFuse(args);
    }

    auto narc = std::make_shared<ntr::fmt::NARC>();
    if(narc->ReadFrom(nitro_path, std::make_shared<ntr::fs::StdioFileHandle>())) {
        g_NitroFsFile = narc;
        g_IsROM = false;
        std::cout << "Mounting plain NARC file: '" << nitro_path << "'..." << std::endl;
        return RunFuse(args);
    }
    else if(narc->ReadFrom(nitro_path, std::make_shared<ntr::fs::StdioFileHandle>(), ntr::fs::FileCompression::LZ77)) {
        g_NitroFsFile = narc;
        g_IsROM = false;
        std::cout << "Mounting LZ77-compressed NARC file: '" << nitro_path << "'..." << std::endl;
        return RunFuse(args);
    }

    std::cerr << "'" << nitro_path << "' is not a recognized NitroFs file!" << std::endl;
    std::cerr << "Supported formats: NDS(i) ROM, NARC/CARC (plain or LZ77-compressed)" << std::endl;
    return 1;
}