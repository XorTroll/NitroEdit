
#pragma once
#include <ntr/ntr_Common.hpp>

namespace ntr {

    struct SWAR : public fs::FileFormat {
        
        struct Header : public CommonHeader<0x52415753 /* "SWAR" */ > {
        };

        struct SampleInfo {
            WaveType wave_type;
            bool has_loop;
            u16 sample_rate;
            u16 time;
            u16 loop_offset;
            u32 non_loop_len;
        };

        struct Sample {
            u32 info_offset;
            SampleInfo info;
            u32 data_size;
        };

        struct DataSection : public CommonBlock<0x41544144 /* "DATA" */ > {
            u8 unk_pad[32];
            u32 sample_count;
        };

        Header header;
        DataSection data;
        std::vector<Sample> samples;

        SWAR() {}
        SWAR(const SWAR&) = delete;

        bool ReadSampleData(const Sample sample, u8 *out_sample_data);
        bool WriteSampleData(const Sample sample, const u8 *sample_data, size_t sample_data_size, const std::string &path, std::shared_ptr<fs::FileHandle> file_handle);

        inline bool WriteSampleData(const Sample sample, const u8 *sample_data, size_t sample_data_size, std::shared_ptr<fs::FileHandle> file_handle) {
            return this->WriteSampleData(sample, sample_data, sample_data_size, this->read_path, file_handle);
        }

        bool ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;

        bool WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override {
            return false;
        }
    };

}