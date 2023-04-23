
#pragma once
#include <ntr/ntr_Include.hpp>

namespace ntr::util {

    template<typename T>
    inline T *NewArray(const size_t count) {
        return new (std::nothrow) T[count]();
    }

    template<typename T>
	inline constexpr T AlignUp(const T value, const u64 align) {
		const auto inv_mask = align - 1;
		return static_cast<T>((value + inv_mask) & ~inv_mask);
	}

    template<typename T>
	inline constexpr T IsAlignedTo(const T value, const u64 align) {
		return (value % align) == 0;
	}

}