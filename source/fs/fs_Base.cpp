#include <fs/fs_Base.hpp>
#include <util/util_String.hpp>

namespace fs {

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