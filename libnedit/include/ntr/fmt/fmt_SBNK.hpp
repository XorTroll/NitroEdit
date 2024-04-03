
#pragma once
#include <ntr/fmt/fmt_Common.hpp>

namespace ntr::fmt {

    struct SBNK : public fs::FileFormat {
        
        struct Header : public CommonHeader<0x4B4E4253 /* "SBNK" */ > {
        };

        struct DataSection : public CommonBlock<0x41544144 /* "DATA" */ > {
            u8 reserved[0x20];
            u32 instrument_count;
        };

        enum class InstrumentType : u8 {
            Invalid = 0,
            Pcm = 1,
            Psg = 2,
            Noise = 3,
            DirectPcm = 4,
            Null = 5,
            DrumSet = 16,
            KeySplit = 17
        };

        static inline constexpr bool IsSimpleInstrument(const InstrumentType type) {
            return (type == InstrumentType::Pcm) || (type == InstrumentType::Psg) || (type == InstrumentType::Noise) || (type == InstrumentType::DirectPcm) || (type == InstrumentType::Null);
        }

        struct InstrumentRecordHeader {
            InstrumentType type;
            u16 offset;
            u8 reserved;
        };

        struct InstrumentData {
            u16 swav;
            u16 swar;
            u8 note;
            u8 attack_rate;
            u8 decay_rate;
            u8 sustain_level;
            u8 release_rate;
            u8 pan;
        };

        struct HeadedInstrumentData {
            u8 unk1;
            u8 unk2;
            InstrumentData data;
        };

        struct SimpleInstrument {
            InstrumentData data;
        };

        struct DrumSetHeader {
            u8 lower_note;
            u8 upper_note;
        };

        struct KeySplitHeader {
            u8 note_region_end_keys[8];
        };
        
        struct KeySplit {
            KeySplitHeader header;
            std::vector<HeadedInstrumentData> data;
        };

        struct DrumSet {
            DrumSetHeader header;
            std::vector<HeadedInstrumentData> data;
        };

        struct InstrumentRecord {
            InstrumentRecordHeader header;
            // ...
            SimpleInstrument simple;
            DrumSet drum_set;
            KeySplit key_split;
        };

        Header header;
        DataSection data;
        std::vector<InstrumentRecord> instruments;

        SBNK() {}
        SBNK(const SBNK&) = delete;

        Result ValidateImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        Result ReadImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override;
        
        Result WriteImpl(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, const fs::FileCompression comp) override {
            NTR_R_FAIL(ResultSBNKWriteNotSupported);
        }
    };

}