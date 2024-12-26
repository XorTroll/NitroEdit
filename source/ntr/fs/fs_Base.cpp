#include <ntr/fs/fs_Base.hpp>
#include <ntr/util/util_String.hpp>

namespace ntr::fs {

    namespace {

        std::string g_ExternalFsDirectory = "nedit_ext_fs";

    }

    void SetExternalFsDirectory(const std::string &path) {
        g_ExternalFsDirectory = path;
    }

    std::string &GetExternalFsDirectory() {
        return g_ExternalFsDirectory;
    }

    ExternalFsFileFormat::ExternalFsFileFormat() : ext_fs_id(static_cast<u32>(rand())) {
        this->ext_fs_root_path = g_ExternalFsDirectory + "/" + std::to_string(this->ext_fs_id);
    }

    std::string GetAbsolutePath(const std::string &path) {
        auto items = util::SplitString(path, '/');
        std::vector<std::string> new_items;
        for(const auto &item : items) {
            if(item == ".") {
                continue;
            }
            else if(item == "..") {
                if(!new_items.empty()) {
                    new_items.pop_back();
                }
            }
            else {
                new_items.push_back(item);
            }
        }
        std::string new_path;
        for(const auto &item : new_items) {
            new_path += item + "/";
        }
        if(!new_path.empty()) {
            new_path.pop_back();
        }
        return new_path;
    }

}