
#pragma once
#include <base_Include.hpp>

namespace util {

    enum class SystemLanguage : u8 {
        Ja,
        En,
        Fr,
        Ge,
        It,
        Es,

        Count
    };

    inline SystemLanguage GetSystemLanguage() {
        return static_cast<SystemLanguage>(PersonalData->language);
    }

}