
#pragma once
#include <ntr/fs/fs_BinaryFile.hpp>

namespace ntr::fmt {

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

}