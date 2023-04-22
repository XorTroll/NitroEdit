
#pragma once
#include <ntr/ntr_Include.hpp>

namespace ntr::util {

    template<typename T>
    inline T *NewArray(const size_t count) {
        return new (std::nothrow) T[count]();
    }

}