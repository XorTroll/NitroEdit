#include <ntr/fmt/fmt_SWAR.hpp>

namespace ntr::fmt {

    bool SWAR::ReadSampleData(const Sample sample, u8 *out_sample_data) {
        fs::BinaryFile bf = {};
        if(!bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, comp)) {
            return false;
        }

        if(!bf.SetAbsoluteOffset(sample.info_offset + sizeof(SampleInfo))) {
            return false;
        }
        if(!bf.ReadData(out_sample_data, sample.data_size)) {
            return false;
        }

        return true;
    }
    
    bool SWAR::WriteSampleData(const Sample sample, const u8 *sample_data, size_t sample_data_size, const std::string &path, std::shared_ptr<fs::FileHandle> file_handle) {
        fs::BinaryFile r_bf = {};
        if(!r_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp)) {
            return false;
        }
        fs::BinaryFile w_bf = {};
        if(!w_bf.Open(file_handle, path, fs::OpenMode::Write, this->comp)) {
            return false;
        }

        const auto pre_sample_offset = 0;
        const auto pre_sample_size = sample.info_offset + sizeof(SampleInfo);
        if(!w_bf.SetAbsoluteOffset(pre_sample_offset)) {
            return false;
        }
        if(!w_bf.CopyFrom(r_bf, pre_sample_size)) {
            return false;
        }

        if(!w_bf.SetAbsoluteOffset(sample.info_offset)) {
            return false;
        }
        if(!w_bf.Write(sample.info)) {
            return false;
        }

        if(!w_bf.WriteData(sample_data, sample_data_size)) {
            return false;
        }
        size_t pad_count = 0;
        while((w_bf.GetAbsoluteOffset() % 0x20) != 0) {
            if(!w_bf.Write(static_cast<u8>(0))) {
                return false;
            }
            pad_count++;
        }

        const auto post_sample_r_offset = pre_sample_size + sample.data_size;
        const auto post_sample_w_offset = pre_sample_size + sample_data_size + pad_count;
        const auto post_sample_size = r_bf.GetSize() - post_sample_r_offset;

        const auto base_offsets_offset = sizeof(Header) + sizeof(DataSection);
        const ssize_t size_diff = sample_data_size + pad_count - sample.data_size;
        for(u32 i = 0; i < this->data.sample_count; i++) {
            auto &cur_sample = this->samples[i];
            if(cur_sample.info_offset > sample.info_offset) {
                cur_sample.info_offset += size_diff;
                if(!w_bf.SetAbsoluteOffset(base_offsets_offset + i * sizeof(Sample::info_offset))) {
                    return false;
                }
                if(!w_bf.Write(cur_sample.info_offset)) {
                    return false;
                }
            }
            else if(cur_sample.info_offset == sample.info_offset) {
                // Other info might have changed...
                cur_sample = sample;
                cur_sample.data_size = sample_data_size;
            }
        }

        if(!r_bf.SetAbsoluteOffset(post_sample_r_offset)) {
            return false;
        }
        if(!w_bf.SetAbsoluteOffset(post_sample_w_offset)) {
            return false;
        }
        if(!w_bf.CopyFrom(r_bf, post_sample_size)) {
            return false;
        }

        this->header.file_size += size_diff;
        this->data.block_size += size_diff;

        if(!w_bf.SetAbsoluteOffset(0)) {
            return false;
        }
        if(!w_bf.Write(this->header)) {
            return false;
        }

        if(!w_bf.Write(this->data)) {
            return false;
        }

        return true;
    }

    bool SWAR::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        if(!bf.Open(file_handle, path, fs::OpenMode::Read, comp)) {
            return false;
        }

        if(!bf.Read(this->header)) {
            return false;
        }
        if(!this->header.IsValid()) {
            return false;
        }

        if(!bf.Read(this->data)) {
            return false;
        }
        if(!this->data.IsValid()) {
            return false;
        }

        this->samples.reserve(this->data.sample_count);
        const auto full_size = bf.GetSize();
        for(u32 i = 0; i < this->data.sample_count; i++) {
            Sample sample = {};
            if(!bf.Read(sample.info_offset)) {
                return false;
            }

            const auto old_offset = bf.GetAbsoluteOffset();
            if(!bf.SetAbsoluteOffset(sample.info_offset)) {
                return false;
            }
            if(!bf.Read(sample.info)) {
                return false;
            }
            if(!bf.SetAbsoluteOffset(old_offset)) {
                return false;
            }
            if((i + 1) < this->data.sample_count) {
                u32 next_sample_info_offset;
                if(!bf.Read(next_sample_info_offset)) {
                    return false;
                }
                sample.data_size = next_sample_info_offset - sample.info_offset - sizeof(SampleInfo);
                if(!bf.SetAbsoluteOffset(old_offset)) {
                    return false;
                }
            }
            else {
                sample.data_size = full_size - sample.info_offset - sizeof(SampleInfo);
            }

            this->samples.push_back(std::move(sample));
        }

        return true;
    }

}