
#pragma once
#include <ntr/ntr_Include.hpp>

namespace ntr::util {

    void LZ77Compress(u8 *&out_data, size_t &out_size, const u8 *data, const size_t data_size);
    void LZ77Decompress(u8 *&out_data, size_t &out_size, const u8 *data);

}