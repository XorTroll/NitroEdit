
#pragma once
#include <ntr/ntr_Include.hpp>

// Define custom NitroEdit PC results, aside from the rest defined in libnedit

constexpr ntr::Result ResultModuleLoadError = 0xd001;
constexpr ntr::Result ResultInvalidModuleSymbols = 0xd002;
constexpr ntr::Result ResultModuleInitializationFailure = 0xd003;

constexpr std::pair<ntr::Result, const char*> ResultDescriptionTable[] = {
    { ResultModuleLoadError, "Error loading module" },
    { ResultInvalidModuleSymbols, "Invalid module symbols" },
    { ResultModuleInitializationFailure, "Unable to get module metadata" }
};

inline constexpr ntr::Result GetResultDescription(const ntr::Result rc, std::string &out_desc) {
    NTR_R_TRY(ntr::GetResultDescription(rc, out_desc));

    for(ntr::u32 i = 0; i < std::size(ResultDescriptionTable); i++) {
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
