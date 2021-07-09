
#pragma once
#include <fs/fs_BinaryFile.hpp>

namespace ntr {

    template<typename T, T BaseMagic>
    struct MagicStartBase {
        T magic;

        inline constexpr bool IsValid() {
            return this->magic == Magic;
        }

        static constexpr T Magic = BaseMagic;
    };

    template<typename T, T BaseMagic1, T BaseMagic2>
    struct MultiMagicStartBase {
        T magic;

        inline constexpr bool IsValid() {
            return (this->magic == Magic1) || (this->magic == Magic2);
        }

        static constexpr T Magic1 = BaseMagic1;
        static constexpr T Magic2 = BaseMagic2;
    };

    template<u32 Magic>
    struct CommonHeader : public MagicStartBase<u32, Magic> {
        u16 byte_order;
        u16 version;
        u32 file_size;
        u16 header_size;
        u16 block_count;
    };

    template<u32 Magic1, u32 Magic2>
    struct CommonMultiMagicHeader : public MultiMagicStartBase<u32, Magic1, Magic2> {
        u16 byte_order;
        u16 version;
        u32 file_size;
        u16 header_size;
        u16 block_count;
    };

    template<u32 Magic>
    struct CommonBlock : public MagicStartBase<u32, Magic> {
        u32 block_size;
    };

    enum class WaveType : u8 {
        PCM8,
        PCM16,
        ADPCM
    };

    static inline constexpr bool ConvertWaveTypeToSoundFormat(const WaveType type, SoundFormat &out_fmt) {
        switch(type) {
            case WaveType::PCM8: {
                out_fmt = SoundFormat_8Bit;
                return true;
            }
            case WaveType::PCM16: {
                out_fmt = SoundFormat_16Bit;
                return true;
            }
            case WaveType::ADPCM: {
                out_fmt = SoundFormat_ADPCM;
                return true;
            }
            default: {
                return false;
            }
        }
        return false;
    }

    static inline constexpr bool ConvertWaveTypeToMicrophoneFormat(const WaveType type, MicFormat &out_fmt) {
        switch(type) {
            case WaveType::PCM8: {
                out_fmt = MicFormat_8Bit;
                return true;
            }
            case WaveType::PCM16: {
                out_fmt = MicFormat_12Bit;
                return true;
            }
            default: {
                return false;
            }
        }
    }

}