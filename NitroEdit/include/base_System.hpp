
#pragma once
#include <base_Include.hpp>
#include <ntr/util/util_System.hpp>

inline ntr::util::SystemLanguage GetSystemLanguage() {
    return static_cast<ntr::util::SystemLanguage>(PersonalData->language);
}