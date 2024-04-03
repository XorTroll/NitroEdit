#include <ntr/fmt/fmt_SBNK.hpp>

namespace ntr::fmt {

    Result SBNK::ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        if(!this->header.IsValid()) {
            NTR_R_FAIL(ResultSBNKInvalidHeader);
        }
        
        NTR_R_TRY(bf.Read(this->data));
        if(!this->data.IsValid()) {
            NTR_R_FAIL(ResultSBNKInvalidDataSection);
        }

        NTR_R_SUCCEED();
    }

    Result SBNK::ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) {
        fs::BinaryFile bf = {};
        NTR_R_TRY(bf.Open(file_handle, path, fs::OpenMode::Read, comp));

        NTR_R_TRY(bf.Read(this->header));
        NTR_R_TRY(bf.Read(this->data));

        this->instruments.clear();
        this->instruments.reserve(this->data.instrument_count);
        NTR_R_TRY(bf.SetAbsoluteOffset(sizeof(Header) + sizeof(DataSection)));
        for(u32 i = 0; i < this->data.instrument_count; i++) {
            InstrumentRecordHeader header = {};
            NTR_R_TRY(bf.Read(header));

            if(header.type == InstrumentType::Invalid) {
                auto &instr = this->instruments.emplace_back();
                instr.header = header;
            }
            else if(IsSimpleInstrument(header.type)) {
                auto &instr = this->instruments.emplace_back();
                instr.header = header;
                instr.simple = {};

                NTR_R_TRY(bf.Read(instr.simple));
            }
            else if(header.type == InstrumentType::DrumSet) {
                auto &instr = this->instruments.emplace_back();
                instr.header = header;
                instr.drum_set = {};

                NTR_R_TRY(bf.Read(instr.drum_set.header));

                const auto instr_count = static_cast<u32>(instr.drum_set.header.upper_note - instr.drum_set.header.lower_note + 1);
                for(u32 i = 0; i < instr_count; i++) {
                    HeadedInstrumentData instr_data;
                    NTR_R_TRY(bf.Read(instr_data));

                    instr.drum_set.data.push_back(instr_data);
                }
            }
            else if(header.type == InstrumentType::KeySplit) {
                auto &instr = this->instruments.emplace_back();
                instr.header = header;
                instr.key_split = {};

                NTR_R_TRY(bf.Read(instr.key_split.header));

                u32 instr_count = 0;
                for(u32 i = 0; i < sizeof(instr.key_split.header.note_region_end_keys); i++) {
                    instr_count++;
                    if(instr.key_split.header.note_region_end_keys[i] == 0) {
                        break;
                    }
                }
                for(u32 i = 0; i < instr_count; i++) {
                    HeadedInstrumentData instr_data;
                    NTR_R_TRY(bf.Read(instr_data));

                    instr.drum_set.data.push_back(instr_data);
                }
            }
        }

        NTR_R_SUCCEED();
    }

}