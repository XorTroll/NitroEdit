
#pragma once
#include <ntr/fmt/fmt_Common.hpp>

namespace ntr::fmt {

    struct STRM : public fs::FileFormat {
        
        struct Header : public CommonHeader<0x4D525453 /* "STRM" */ > {
        };

        struct HeadSection : public CommonBlock<0x44414548 /* "HEAD" */ > {
            WaveType type;
            bool has_loop;
            u8 channel_count;
            u8 unk_pad;
            u16 sample_rate;
            u16 time;
            u32 loop_offset;
            u32 sample_count;
            u32 data_offset;
            u32 block_count;
            u32 block_size;
            u32 samples_per_block;
            u32 last_block_size;
            u32 samples_per_last_block;
            u8 reserved[32];
        };

        struct DataSection : public CommonBlock<0x41544144 /* "DATA" */ > {
        };

        Header header;
        HeadSection head;
        DataSection data;

        STRM() {}
        STRM(const STRM&) = delete;

        inline constexpr size_t GetBlockSize(const u32 block_idx) {
            if(block_idx < this->head.block_count) {
                if(block_idx == this->head.block_count - 1) {
                    return this->head.last_block_size;
                }
                else {
                    return this->head.block_size;
                }
            }
            else {
                return 0;
            }
        }

        /* TODO: read/write stream data blocks */

        bool ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        
        bool WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override {
            return false;
        }
    };

}