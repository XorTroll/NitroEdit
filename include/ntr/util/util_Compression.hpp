
#pragma once
#include <ntr/ntr_Include.hpp>

namespace ntr::util {

    enum class LzVersion : u8 {
        Invalid = 0,
        LZ10 = 0x10,
        LZ11 = 0x11
    };

    // LZ10 size is encoded in 3 bytes...
    constexpr size_t MaximumLZ10CompressSize = 1ul << static_cast<size_t>(3 * CHAR_BIT);

    // ...while LZ11 size is encoded in 4 bytes
    constexpr u64 MaximumLZ11CompressSize = 1ull << static_cast<u64>(4 * CHAR_BIT);

    constexpr u32 LZ10RepeatSize = 18;
    constexpr u32 MaximumLZ11RepeatSize = 65809;
    constexpr u32 DefaultRepeatSize = LZ10RepeatSize;

    Result LzValidateCompressed(const u32 lz_header, LzVersion &out_ver);

    Result LzCompress(const u8 *data, const size_t data_size, const LzVersion ver, const u32 repeat_size, u8 *&out_data, size_t &out_size);

    inline Result LzCompressDefault(const u8 *data, const size_t data_size, const LzVersion ver, u8 *&out_data, size_t &out_size) {
        return LzCompress(data, data_size, ver, DefaultRepeatSize, out_data, out_size);
    }

    Result LzDecompress(const u8 *data, u8 *&out_data, size_t &out_size, LzVersion &out_ver, size_t &out_used_data_size);

}