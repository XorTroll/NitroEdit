
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <codecvt>
#include <locale>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <functional>
#include <climits>
#include <cmath>
#include <stack>

#define ATTR_PACKED __attribute__((packed))

namespace ntr {

    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    class ScopeGuard {
        public:
            using Fn = std::function<void()>;

        private:
            bool run;
            Fn exit_fn;

        public:
            ScopeGuard(Fn &&fn) : run(true), exit_fn(fn) {}

            ~ScopeGuard() {
                if(this->run) {
                    this->exit_fn();
                }
            }

            inline void Cancel() {
                this->run = false;
            }
    };

}

#define NTR_R_SUCCEED() return ::ntr::ResultSuccess

#define NTR_R_FAIL(rc) { \
    const auto _tmp_rc = (rc); \
    return _tmp_rc; \
}

#define NTR_R_TRY(rc) { \
    const auto _tmp_rc = (rc); \
    if(_tmp_rc.IsFailure()) { \
        return _tmp_rc; \
    } \
}

namespace ntr {

    struct Result {
        using BaseType = u16;

        BaseType value;

        static constexpr BaseType SuccessValue = BaseType();

        constexpr Result() : value() {}
        constexpr Result(const BaseType val) : value(val) {}

        inline constexpr bool IsSuccess() const {
            return this->value == SuccessValue;
        }

        inline constexpr bool IsFailure() const {
            return !this->IsSuccess();
        }

        inline std::string GetDescription() const;
    };

    constexpr Result ResultSuccess = 0;
    constexpr Result ResultUnknownResult = 0x0002;

    constexpr Result ResultUnsupportedPixelFormat = 0x0101;
    constexpr Result ResultInvalidSizeForPixelFormat = 0x0102;
    constexpr Result ResultColorPaletteAllocationFailure = 0x0103;
    constexpr Result ResultInvalidPixelFormat = 0x0104;
    constexpr Result ResultInvalidCharacterFormat = 0x0105;
    constexpr Result ResultInvalidScreenData = 0x0106;
    constexpr Result ResultScreenDataGenerationUnimplemented = 0x0107;
    constexpr Result ResultUnexpectedBannerIconDimensions = 0x0108;

    constexpr Result ResultInvalidFile = 0x0201;
    constexpr Result ResultEndOfData = 0x0202;
    constexpr Result ResultReadNotSupported = 0x0203;
    constexpr Result ResultWriteNotSupported = 0x0204;
    constexpr Result ResultUnexpectedReadSize = 0x0205;
    constexpr Result ResultFileNotCompressed = 0x0206;
    constexpr Result ResultInvalidEndSeekOffset = 0x0207;
    constexpr Result ResultInvalidFileOpenMode = 0x0208;
    constexpr Result ResultUnableToOpenStdioFile = 0x0209;
    constexpr Result ResultInvalidSeekPosition = 0x020a;
    constexpr Result ResultUnableToSeekStdioFile = 0x020b;
    constexpr Result ResultUnableToReadStdioFile = 0x020c;
    constexpr Result ResultUnableToWriteStdioFile = 0x020d;
    constexpr Result ResultUnableToCloseStdioFile = 0x020e;
    constexpr Result ResultUnableToCreateStdioDirectory = 0x020f;
    constexpr Result ResultUnableToDeleteStdioFile = 0x0210;

    constexpr Result ResultNitroFsDirectoryNotFound = 0x0301;
    constexpr Result ResultNitroFsFileNotFound = 0x0302;

    constexpr Result ResultBMGInvalidHeader = 0x0401;
    constexpr Result ResultBMGInvalidInfoSection = 0x0402;
    constexpr Result ResultBMGInvalidDataSection = 0x0403;
    constexpr Result ResultBMGInvalidUnsupportedCharacterFormat = 0x0404;
    constexpr Result ResultBMGInvalidMessageAttributes = 0x0405;

    constexpr Result ResultNCGRInvalidHeader = 0x0501;
    constexpr Result ResultNCGRInvalidCharacterDataBlock = 0x0502;
    constexpr Result ResultNCGRInvalidCharacterPositionBlock = 0x0503;

    constexpr Result ResultNCLRInvalidHeader = 0x0601;
    constexpr Result ResultNCLRInvalidPaletteDataBlock = 0x0602;

    constexpr Result ResultNSCRInvalidHeader = 0x0701;
    constexpr Result ResultNSCRInvalidScreenDataBlock = 0x0702;
    constexpr Result ResultNSCRWriteNotSupported = 0x0703;

    constexpr Result ResultNARCInvalidHeader = 0x0801;
    constexpr Result ResultNARCInvalidFileAllocationTableBlock = 0x0802;
    constexpr Result ResultNARCInvalidFileNameTableBlock = 0x0803;
    constexpr Result ResultNARCInvalidFileImageBlock = 0x0804;

    constexpr Result ResultSDATInvalidHeader = 0x0901;
    constexpr Result ResultSDATInvalidSymbolBlock = 0x0902;
    constexpr Result ResultSDATInvalidInfoBlock = 0x0903;
    constexpr Result ResultSDATInvalidFileAllocationTableBlock = 0x0904;
    constexpr Result ResultSDATInvalidFileBlock = 0x0905;
    constexpr Result ResultSDATInvalidSymbols = 0x0906;
    constexpr Result ResultSDATEntryNotFound = 0x0907;
    constexpr Result ResultSDATInvalidEntryFormat = 0x0908;
    
    constexpr Result ResultSSEQInvalidHeader = 0x0a01;
    constexpr Result ResultSSEQInvalidDataSection = 0x0a02;
    constexpr Result ResultSSEQWriteNotSupported = 0x0a03;
    
    constexpr Result ResultSBNKInvalidHeader = 0x0b01;
    constexpr Result ResultSBNKInvalidDataSection = 0x0b02;
    constexpr Result ResultSBNKWriteNotSupported = 0x0b03;

    constexpr Result ResultSWARInvalidHeader = 0x0c01;
    constexpr Result ResultSWARInvalidDataSection = 0x0c02;
    constexpr Result ResultSWARWriteNotSupported = 0x0c03;
    
    constexpr Result ResultSTRMInvalidHeader = 0x0d01;
    constexpr Result ResultSTRMInvalidHeadSection = 0x0d02;
    constexpr Result ResultSTRMInvalidDataSection = 0x0d03;
    constexpr Result ResultSTRMWriteNotSupported = 0x0d04;

    constexpr Result ResultROMInvalidUnitCode = 0x0e01;
    constexpr Result ResultROMInvalidNintendoLogoCRC16 = 0x0e02;

    constexpr Result ResultCompressionInvalidLzFormat = 0x0f01;
    constexpr Result ResultCompressionTooBigCompressSize = 0x0f02;
    constexpr Result ResultCompressionInvalidRepeatSize = 0x0f03;

    constexpr Result ResultUtilityInvalidSections = 0x1001;

    constexpr std::pair<Result, const char*> ResultDescriptionTable[] = {
        { ResultSuccess, "Success" },
        { ResultUnknownResult, "Unknown result value" },

        { ResultUnsupportedPixelFormat, "Unsupported pixel format" },
        { ResultInvalidSizeForPixelFormat, "Invalid size for given pixel format" },
        { ResultColorPaletteAllocationFailure, "Unable to allocate color in palette" },
        { ResultInvalidPixelFormat, "Invalid pixel format" },
        { ResultInvalidCharacterFormat, "Invalid (graphics) character format" },
        { ResultInvalidScreenData, "Invalid screen (NSCR) data" },
        { ResultScreenDataGenerationUnimplemented, "Unimplemented feature: screen data saving/generation" },
        { ResultUnexpectedBannerIconDimensions, "Unexpected banner icon dimensions" },

        { ResultInvalidFile, "Invalid file" },
        { ResultEndOfData, "End of data (EOF)" },
        { ResultReadNotSupported, "Read not supported in file" },
        { ResultWriteNotSupported, "Write not supported in file" },
        { ResultUnexpectedReadSize, "Got unexpected read size" },
        { ResultFileNotCompressed, "File is not compressed" },
        { ResultInvalidEndSeekOffset, "Invalid offset for end seeking (must be 0)" },
        { ResultInvalidFileOpenMode, "Invalid file open mode" },
        { ResultUnableToOpenStdioFile, "Unable to open stdio file" },
        { ResultInvalidSeekPosition, "Invalid seek position" },
        { ResultUnableToSeekStdioFile, "Unable to seek stdio file" },
        { ResultUnableToReadStdioFile, "Unable to read stdio file" },
        { ResultUnableToWriteStdioFile, "Unable to write stdio file" },
        { ResultUnableToCloseStdioFile, "Unable to close stdio file" },
        { ResultUnableToCreateStdioDirectory, "Unable to create stdio directory" },
        { ResultUnableToDeleteStdioFile, "Unable to delete stdio file" },

        { ResultNitroFsDirectoryNotFound, "NitroFs directory not found" },
        { ResultNitroFsFileNotFound, "NitroFs file not found" },

        { ResultBMGInvalidHeader, "Invalid BMG header" },
        { ResultBMGInvalidInfoSection, "Invalid BMG INFO section" },
        { ResultBMGInvalidDataSection, "Invalid BMG DATA section" },
        { ResultBMGInvalidUnsupportedCharacterFormat, "Unsupported BMG character format" },
        { ResultBMGInvalidMessageAttributes, "Invalid BMG message attributes" },

        { ResultNCGRInvalidHeader, "Invalid NCGR header" },
        { ResultNCGRInvalidCharacterDataBlock, "Invalid NCGR character data block" },
        { ResultNCGRInvalidCharacterPositionBlock, "Invalid NCGR character position block" },

        { ResultNCLRInvalidHeader, "Invalid NCLR header" },
        { ResultNCLRInvalidPaletteDataBlock, "Invalid NCGR palette data block" },

        { ResultNSCRInvalidHeader, "Invalid NSCR header" },
        { ResultNSCRInvalidScreenDataBlock, "Invalid NSCR screen data block" },
        { ResultNSCRWriteNotSupported, "Unsupported feature: writing to NSCR" },

        { ResultNARCInvalidHeader, "Invalid NARC header" },
        { ResultNARCInvalidFileAllocationTableBlock, "Invalid NARC file allocation table block" },
        { ResultNARCInvalidFileNameTableBlock, "Invalid NARC file name table block" },
        { ResultNARCInvalidFileImageBlock, "Invalid NARC file image block" },

        { ResultSDATInvalidHeader, "Invalid SDAT header" },
        { ResultSDATInvalidSymbolBlock, "Invalid SDAT symbol block" },
        { ResultSDATInvalidInfoBlock, "Invalid SDAT info block" },
        { ResultSDATInvalidFileAllocationTableBlock, "Invalid SDAT file allocation table block" },
        { ResultSDATInvalidFileBlock, "Invalid SDAT file block" },
        { ResultSDATInvalidSymbols, "Invalid SDAT symbols" },
        { ResultSDATEntryNotFound, "SDAT entry not found" },
        { ResultSDATInvalidEntryFormat, "Invalid SDAT entry format" },

        { ResultSSEQInvalidHeader, "Invalid SSEQ header" },
        { ResultSSEQInvalidDataSection, "Invalid SSEQ data section" },
        { ResultSSEQWriteNotSupported, "Unsupported feature: writing to SSEQ" },

        { ResultSBNKInvalidHeader, "Invalid SBNK header" },
        { ResultSBNKInvalidDataSection, "Invalid SBNK data section" },
        { ResultSBNKWriteNotSupported, "Unsupported feature: writing to SBNK" },

        { ResultSWARInvalidHeader, "Invalid SWAR header" },
        { ResultSWARInvalidDataSection, "Invalid SWAR data section" },
        { ResultSWARWriteNotSupported, "Unsupported feature: writing to SWAR" },

        { ResultSTRMInvalidHeader, "Invalid STRM header" },
        { ResultSTRMInvalidHeadSection, "Invalid STRM head section" },
        { ResultSTRMInvalidDataSection, "Invalid STRM data section" },
        { ResultSTRMWriteNotSupported, "Unsupported feature: writing to STRM" },

        { ResultUtilityInvalidSections, "Invalid DWC utility sections" }
    };

    inline constexpr Result GetResultDescription(const Result rc, std::string &out_desc) {
        for(u32 i = 0; i < std::size(ResultDescriptionTable); i++) {
            if(ResultDescriptionTable[i].first.value == rc.value) {
                out_desc = ResultDescriptionTable[i].second;
                NTR_R_SUCCEED(); 
            }
        }

        NTR_R_FAIL(ResultUnknownResult);
    }

    inline std::string Result::GetDescription() const {
        std::string desc = "<unknown result: " + std::to_string(this->value) + ">";
        GetResultDescription(*this, desc);
        return desc;
    }

}

inline constexpr size_t operator ""_KB(unsigned long long n) {
    return n * 0x400;
}

inline constexpr size_t operator ""_MB(unsigned long long n) {
    return operator ""_KB(n) * 0x400;
}

inline constexpr size_t operator ""_GB(unsigned long long n) {
    return operator ""_MB(n) * 0x400;
}

#define NTR_ENUM_BIT_OPERATORS(enum_type, base_type) \
inline constexpr enum_type operator|(enum_type lhs, enum_type rhs) { \
    return static_cast<enum_type>(static_cast<base_type>(lhs) | static_cast<base_type>(rhs)); \
} \
inline constexpr enum_type operator&(enum_type lhs, enum_type rhs) { \
    return static_cast<enum_type>(static_cast<base_type>(lhs) & static_cast<base_type>(rhs)); \
}

#define NTR_BITMASK(n) (1 << n)