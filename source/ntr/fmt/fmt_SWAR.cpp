#include <ntr/fmt/fmt_SWAR.hpp>

void Log(const std::string &msg);

namespace ntr::fmt {

    Result SWAR::ReadSampleData(const Sample sample, u8 *out_sample_data) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp));

        NTR_R_TRY(bf.SetAbsoluteOffset(sample.info_offset + sizeof(SampleInfo)));
        NTR_R_TRY(bf.ReadDataExact(out_sample_data, sample.data_size));

        NTR_R_SUCCEED();
    }
    
    Result SWAR::WriteSampleData(const Sample sample, const u8 *sample_data, size_t sample_data_size, const std::string &path, std::shared_ptr<fs::FileHandle> file_handle) {
        fs::BinaryFile r_bf = {};
        NTR_R_TRY(r_bf.Open(this->read_file_handle, this->read_path, fs::OpenMode::Read, this->comp));
        fs::BinaryFile w_bf = {};
        NTR_R_TRY(w_bf.Open(file_handle, path, fs::OpenMode::Write, this->comp));

        const auto pre_sample_offset = 0;
        const auto pre_sample_size = sample.info_offset + sizeof(SampleInfo);
        NTR_R_TRY(w_bf.SetAbsoluteOffset(pre_sample_offset));
        NTR_R_TRY(w_bf.CopyFrom(r_bf, pre_sample_size));

        NTR_R_TRY(w_bf.SetAbsoluteOffset(sample.info_offset));
        NTR_R_TRY(w_bf.Write(sample.info));

        NTR_R_TRY(w_bf.WriteData(sample_data, sample_data_size));
        size_t pad_count = 0;
        w_bf.WriteEnsureAlignment(0x20, pad_count);

        const auto post_sample_r_offset = pre_sample_size + sample.data_size;
        const auto post_sample_w_offset = pre_sample_size + sample_data_size + pad_count;
        size_t r_bf_size;
        NTR_R_TRY(r_bf.GetSize(r_bf_size));
        const auto post_sample_size = r_bf_size - post_sample_r_offset;

        const auto base_offsets_offset = sizeof(Header) + sizeof(DataSection);
        const ssize_t size_diff = sample_data_size + pad_count - sample.data_size;
        for(u32 i = 0; i < this->data.sample_count; i++) {
            auto &cur_sample = this->samples[i];
            if(cur_sample.info_offset > sample.info_offset) {
                cur_sample.info_offset += size_diff;
                NTR_R_TRY(w_bf.SetAbsoluteOffset(base_offsets_offset + i * sizeof(Sample::info_offset)));
                NTR_R_TRY(w_bf.Write(cur_sample.info_offset));
            }
            else if(cur_sample.info_offset == sample.info_offset) {
                // Other info might have changed...
                cur_sample = sample;
                cur_sample.data_size = sample_data_size;
            }
        }

        NTR_R_TRY(r_bf.SetAbsoluteOffset(post_sample_r_offset));
        NTR_R_TRY(w_bf.SetAbsoluteOffset(post_sample_w_offset));
        NTR_R_TRY(w_bf.CopyFrom(r_bf, post_sample_size));

        this->header.file_size += size_diff;
        this->data.block_size += size_diff;

        NTR_R_TRY(w_bf.SetAbsoluteOffset(0));
        NTR_R_TRY(w_bf.Write(this->header));

        NTR_R_TRY(w_bf.Write(this->data));

        NTR_R_SUCCEED();
    }

    Result SWAR::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultSWARInvalidHeader);
        }

        NTR_R_TRY(bf.Read(this->data));
        if(!this->data.IsValid()) {
            NTR_R_FAIL(ResultSWARInvalidDataSection);
        }

        NTR_R_SUCCEED();
    }

    Result SWAR::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        this->samples.clear();
        this->samples.reserve(this->data.sample_count);

        size_t f_size;
        NTR_R_TRY(bf.GetSize(f_size));

        NTR_R_TRY(bf.SetAbsoluteOffset(sizeof(Header) + sizeof(DataSection)));
        for(u32 i = 0; i < this->data.sample_count; i++) {
            Sample sample = {};
            NTR_R_TRY(bf.Read(sample.info_offset));

            size_t old_offset;
            NTR_R_TRY(bf.GetAbsoluteOffset(old_offset));
        
            NTR_R_TRY(bf.SetAbsoluteOffset(sample.info_offset));
            NTR_R_TRY(bf.Read(sample.info));
            NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));

            if((i + 1) < this->data.sample_count) {
                u32 next_sample_info_offset;
                NTR_R_TRY(bf.Read(next_sample_info_offset));
                sample.data_size = next_sample_info_offset - sample.info_offset - sizeof(SampleInfo);
                NTR_R_TRY(bf.SetAbsoluteOffset(old_offset));
            }
            else {
                sample.data_size = f_size - sample.info_offset - sizeof(SampleInfo);
            }

            this->samples.push_back(std::move(sample));
        }

        NTR_R_SUCCEED();
    }

}