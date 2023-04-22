
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

}