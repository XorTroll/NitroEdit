#include <ntr/util/util_String.hpp>

namespace ntr::util {

    std::vector<std::string> SplitString(const std::string &str, const char separator) {
        std::vector<std::string> items;
        size_t prev_pos = 0;
        size_t pos = 0;
        while((pos = str.find(separator, pos)) != std::string::npos) {
            items.push_back(str.substr(prev_pos, pos - prev_pos));
            prev_pos = ++pos;
        }
        items.push_back(str.substr(prev_pos, pos - prev_pos));
        return items;
    }

}