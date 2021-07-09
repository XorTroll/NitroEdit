
#pragma once
#include <base_Include.hpp>

namespace util {

    template<typename T>
    inline T *NewArray(const size_t count) {
        return new (std::nothrow) T[count]();
    }

}