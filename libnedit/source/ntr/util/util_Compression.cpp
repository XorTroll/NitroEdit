#include <ntr/util/util_Compression.hpp>
#include <ntr/util/util_Memory.hpp>

namespace ntr::util {

    namespace {

        inline constexpr void GenerateNeedleTable(const u8 *needle, const size_t needle_size, u8 (&out_table)[0x100]) {
            std::memset(out_table, static_cast<u8>(needle_size & 0xff), sizeof(out_table));
            for(size_t i = 0; i < needle_size; i++) {
                out_table[needle[i]] = needle_size - i;
            }
        }

        bool SearchOne(const u8 *haystack, const size_t haystack_size, const u8 *needle, const size_t needle_size, const u8 needle_table[0x100], size_t &out) {
            size_t cur = 0;
            while((haystack_size - cur) >= needle_size) {
                for(size_t i = 0; i < needle_size; i++) {
                    const auto j = needle_size - i - 1;
                    if(haystack[cur + j] == needle[j]) {
                        out = cur;
                        return true;
                    }
                }

                cur += needle_table[haystack[cur + needle_size - 1]];
            }
            
            return false;
        }

        std::vector<size_t> Search(const u8 *haystack, const size_t haystack_size, const u8 *needle, const size_t needle_size) {
            u8 needle_table[0x100] = {};
            GenerateNeedleTable(needle, needle_size, needle_table);

            size_t cur = 0;
            std::vector<size_t> offsets;
            while((cur + needle_size) < haystack_size) {
                size_t found_offset;
                if(SearchOne(haystack + cur, haystack_size - cur, needle, needle_size, needle_table, found_offset)) {
                    offsets.push_back(found_offset);
                    cur += found_offset + needle_size + 1;
                }
                else {
                    break;
                }
            }

            return offsets;
        }

        bool FindLongestMatch(const u8 *data, const size_t data_size, const size_t offset, const size_t max, size_t &out_offset, size_t &out_size) {
            if((offset < 4) || ((data_size - offset) < 4)) {
                return false;
            }

            size_t longest_offset = 0;
            size_t longest_size = 0;
            size_t start = 0;
            if(offset > 0x1000) {
                start = offset - 0x1000;
            }

            const auto offsets = Search(data + start, offset + 2 - start, data + offset, 3);
            for(const auto &found_offset: offsets) {
                size_t size = 0;
                for(size_t i = 0; i < (data_size - offset); i++) {
                    const auto p = i + offset;
                    if(size == max) {
                        out_offset = start + found_offset;
                        out_size = size;
                        return true;
                    }
                    if(data[p] != data[start + found_offset + i]) {
                        break;
                    }
                    size++;
                }
                if(size > longest_size) {
                    longest_offset = found_offset;
                    longest_size = size;
                }
            }

            if(longest_size < 3) {
                return false;
            }
            else {
                out_offset = start + longest_offset;
                out_size = longest_size;
                return true;
            }
        }

    }

    Result LzValidateCompressed(const u32 lz_header, LzVersion &out_ver) {
        out_ver = static_cast<LzVersion>(lz_header & 0xff);
        if((out_ver != LzVersion::LZ10) && (out_ver != LzVersion::LZ11)) {
            NTR_R_FAIL(ResultCompressionInvalidLzFormat);
        }

        NTR_R_SUCCEED();
    }

    // TODO: proper buffer readers?

    Result LzCompress(const u8 *data, const size_t data_size, const LzVersion ver, const u32 repeat_size, u8 *&out_data, size_t &out_size) {
        const auto tmp_out_size_estimate = data_size + data_size / 8 + 4;
        auto tmp_out_data = util::NewArray<u8>(tmp_out_size_estimate);
        ScopeGuard on_exit_cleanup_1([&]() {
            delete[] tmp_out_data;
        });

        size_t out_offset = 0;
        if(ver == LzVersion::LZ10) {
            if(data_size > MaximumLZ10CompressSize) {
                NTR_R_FAIL(ResultCompressionTooBigCompressSize);
            }
            if(repeat_size != LZ10RepeatSize) {
                NTR_R_FAIL(ResultCompressionInvalidRepeatSize);
            }

            const auto header_32 = static_cast<u32>(ver) + static_cast<u32>(data_size << 8);
            *reinterpret_cast<u32*>(tmp_out_data + out_offset) = header_32;
            out_offset += sizeof(u32);
        }
        else if(ver == LzVersion::LZ11) {
            if(data_size > MaximumLZ11CompressSize) {
                NTR_R_FAIL(ResultCompressionTooBigCompressSize);
            }
            if(repeat_size > MaximumLZ11RepeatSize) {
                NTR_R_FAIL(ResultCompressionInvalidRepeatSize);
            }

            *reinterpret_cast<u32*>(tmp_out_data + out_offset) = static_cast<u32>(ver);
            out_offset += sizeof(u32);
            *reinterpret_cast<u32*>(tmp_out_data + out_offset) = static_cast<u32>(data_size);
            out_offset += sizeof(u32);
        }
        else {
            NTR_R_FAIL(ResultCompressionInvalidLzFormat);
        }

        size_t offset = 0;
        u8 byte = 0;
        ssize_t index = 7;
        auto cmp_buf = util::NewArray<u8>(tmp_out_size_estimate);
        size_t cmp_buf_offset = 0;
        ScopeGuard on_exit_cleanup_2([&]() {
            delete[] cmp_buf;
        });
        while(offset < data_size) {
            size_t find_offset;
            size_t find_size;
            if(FindLongestMatch(data, data_size, offset, repeat_size, find_offset, find_size)) {
                const auto lz_offset = offset - find_offset - 1;
                byte |= static_cast<u8>(1 << index);
                index--;

                if(ver == LzVersion::LZ10) {
                    const auto l = find_size - 0x3;
                    cmp_buf[cmp_buf_offset] = static_cast<u8>((lz_offset >> 8) & 0xff) + static_cast<u8>((l << 4) & 0xff);
                    cmp_buf_offset++;
                    cmp_buf[cmp_buf_offset] = static_cast<u8>(lz_offset & 0xff);
                    cmp_buf_offset++;
                }
                else if(ver == LzVersion::LZ11) {
                    if(find_size < 0x11) {
                        const auto l = find_size - 0x1;
                        cmp_buf[cmp_buf_offset] = static_cast<u8>((lz_offset >> 8) & 0xff) + static_cast<u8>((l << 4) & 0xff);
                        cmp_buf_offset++;
                        cmp_buf[cmp_buf_offset] = static_cast<u8>(lz_offset & 0xff);
                        cmp_buf_offset++;
                    }
                    else if(find_size < 0x111) {
                        const auto l = find_size - 0x11;
                        cmp_buf[cmp_buf_offset] = static_cast<u8>((l >> 4) & 0xff);
                        cmp_buf_offset++;
                        cmp_buf[cmp_buf_offset] = static_cast<u8>((lz_offset >> 8) & 0xff) + static_cast<u8>((l << 4) & 0xff);
                        cmp_buf_offset++;
                        cmp_buf[cmp_buf_offset] = static_cast<u8>(lz_offset & 0xff);
                        cmp_buf_offset++;
                    }
                    else {
                        const auto l = find_size - 0x111;
                        cmp_buf[cmp_buf_offset] = static_cast<u8>((l >> 12) & 0xff) + 0x10;
                        cmp_buf_offset++;
                        cmp_buf[cmp_buf_offset] = static_cast<u8>((l >> 4) & 0xff);
                        cmp_buf_offset++;
                        cmp_buf[cmp_buf_offset] = static_cast<u8>((lz_offset >> 8) & 0xff) + static_cast<u8>((l << 4) & 0xff);
                        cmp_buf_offset++;
                        cmp_buf[cmp_buf_offset] = static_cast<u8>(lz_offset & 0xff);
                        cmp_buf_offset++;
                    }
                }
                offset += find_size;
            }
            else {
                index--;
                cmp_buf[cmp_buf_offset] = data[offset];
                offset++;
                cmp_buf_offset++;
            }

            if(index < 0) {
                tmp_out_data[out_offset] = byte;
                out_offset++;
                if(cmp_buf_offset > 0) {
                    std::memcpy(tmp_out_data + out_offset, cmp_buf, cmp_buf_offset);
                    out_offset += cmp_buf_offset;
                }
                cmp_buf_offset = 0;
                byte = 0;
                index = 7;
            }
        }

        if(cmp_buf_offset > 0) {
            tmp_out_data[out_offset] = byte;
            out_offset++;
            std::memcpy(tmp_out_data + out_offset, cmp_buf, cmp_buf_offset);
            out_offset += cmp_buf_offset;
        }
        tmp_out_data[out_offset] = 0xff;
        out_offset++;

        out_data = util::NewArray<u8>(out_offset);
        out_size = out_offset;
        std::memcpy(out_data, tmp_out_data, out_offset);
        NTR_R_SUCCEED();
    }

    /*
    
    lz10 functionality

    u32 header --> top 3 bytes * 0x100 == dec_size, low byte == lz version (0x10)

    loop:
        u8 control_byte
        for each bit in control_byte:
            if bit not set:
                u8 raw_val, append to decompressed buffer
            if bit set:
                u16 data --> 4 high bits == copy length, 12 low bits == displacement

                copy "copy_len" bytes from dec_buffer[]
    
    */

    Result LzDecompress(const u8 *data, u8 *&out_data, size_t &out_size, LzVersion &out_ver, size_t &out_used_data_size) {
        size_t offset = 0;
        const auto lz_header = *reinterpret_cast<const u32*>(data);
        offset += sizeof(u32);

        LzVersion ver;
        NTR_R_TRY(LzValidateCompressed(lz_header, ver));

        out_size = lz_header >> 8;
        out_ver = ver;
        if((out_size == 0) && (ver == LzVersion::LZ11)) {
            out_size = *reinterpret_cast<const u32*>(data + offset);
            offset += sizeof(u32);
        }
        out_data = util::NewArray<u8>(out_size);

        size_t out_offset = 0;
        while(out_offset < out_size) {
            const auto cur_byte = data[offset];
            offset++;

            for(u8 i = 0; i < 8; i++) {
                if(out_offset >= out_size) {
                    break;
                }

                const auto bit = 8 - (i + 1);
                if((cur_byte >> bit) & 1) {
                    const size_t msb_len = data[offset];
                    offset++;
                    const size_t lsb = data[offset]; // 4 bits (length) + 12 bits (disp)
                    offset++;

                    auto length = msb_len >> 4; // 4 high bits
                    auto disp = ((msb_len & 15) << 8) + lsb; // 4 low bits * 0x100 + lsb

                    if(ver == LzVersion::LZ10) {
                        length += 3;
                    }
                    else if(length > 1) {
                        length++;
                    }
                    else if(length == 0) {
                        length = (msb_len & 15) << 4;
                        length += lsb >> 4;
                        length += 0x11;
                        const size_t msb = data[offset];
                        offset++;
                        disp = ((lsb & 15) << 8) + msb;
                    }
                    else {
                        length = (msb_len & 15) << 12;
                        length += lsb << 4;
                        const size_t byte_1 = data[offset];
                        offset++;
                        const size_t byte_2 = data[offset];
                        offset++;
                        length += byte_1 >> 4;
                        length += 0x111;
                        disp = ((byte_1 & 15) << 8) + byte_2;
                    }

                    const auto start_offset = out_offset - disp - 1;
                    for(size_t j = 0; j < length; j++) {
                        const auto val = out_data[start_offset + j];
                        out_data[out_offset] = val;
                        out_offset++;
                    }
                }
                else {
                    out_data[out_offset] = data[offset];
                    offset++;
                    out_offset++;
                }
            }
        }

        out_used_data_size = offset;
        NTR_R_SUCCEED();
    }

}