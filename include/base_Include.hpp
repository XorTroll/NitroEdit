
#pragma once
#include <ntr/ntr_Include.hpp>
#include <nds.h>
#include <fat.h>

#define ATTR_NORETURN __attribute__((noreturn))

#define NEDIT_DEBUG

// Define custom NitroEdit results, aside from the rest defined in libnedit

constexpr ntr::Result ResultUiSpriteNotCreated = 0xa001;
constexpr ntr::Result ResultUiUnableToAllocateSpriteIndex = 0xa002;
constexpr ntr::Result ResultUiSpriteInvalidGraphicsData = 0xa003;
constexpr ntr::Result ResultUiUnableToLoadFont = 0xa004;
constexpr ntr::Result ResultUiLodepngDecodeError = 0xa005;

constexpr ntr::Result ResultEditBMGInvalidEscapeByte = 0xa101;
constexpr ntr::Result ResultEditBMGUnexpectedEscapeOpen = 0xa101;
constexpr ntr::Result ResultEditBMGUnexpectedEscapeClose = 0xa101;
constexpr ntr::Result ResultEditBMGUnclosedEscape = 0xa101;

constexpr std::pair<ntr::Result, const char*> ResultDescriptionTable[] = {
    { ResultUiSpriteNotCreated, "UI sprite not created before loading" },
    { ResultUiUnableToAllocateSpriteIndex, "Unable to allocate UI sprite index" },
    { ResultUiSpriteInvalidGraphicsData, "Unable to allocate UI sprite graphics data" },
    { ResultUiUnableToLoadFont, "Unable to load text font" },
    { ResultUiLodepngDecodeError, "lodepng PNG decode error" }
};

inline constexpr ntr::Result GetResultDescription(const ntr::Result rc, std::string &out_desc) {
    NTR_R_TRY(ntr::GetResultDescription(rc, out_desc));

    for(u32 i = 0; i < std::size(ResultDescriptionTable); i++) {
        if(ResultDescriptionTable[i].first.value == rc.value) {
            out_desc = ResultDescriptionTable[i].second;
            NTR_R_SUCCEED(); 
        }
    }

    NTR_R_FAIL(ntr::ResultUnknownResult);
}

inline std::string FormatResult(const ntr::Result rc) {
    std::string desc = "<unknown error: " + std::to_string(rc.value) + ">";
    ::GetResultDescription(rc, desc);
    return desc;
}

void ATTR_NORETURN OnCriticalError(const std::string &err);

#define ASSERT_SUCCESSFUL(expr) ({ \
    const auto _expr_rc = (expr); \
    if(_expr_rc.IsFailure()) { \
        ::OnCriticalError("'" #expr "' failed: '" + FormatResult(_expr_rc) + "'"); \
    } \
})

#define ASSERT_TRUE_MSG(expr, err) ({ \
    const auto _expr_val = (expr); \
    if(!_expr_val) { \
        ::OnCriticalError(err); \
    } \
})

#define ASSERT_TRUE(expr) ASSERT_TRUE_MSG(expr, "'" #expr "' failed")