
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <codecvt>
#include <locale>
#include <nds.h>
#include <fat.h>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <functional>
#include <climits>
#include <cmath>
#include <stack>

#define ATTR_PACKED __attribute__((packed))
#define ATTR_NORETURN __attribute__((noreturn))

void ATTR_NORETURN OnCriticalError(const std::string &err);

#define ASSERT_TRUE_MSG(expr, err) ({ \
    const auto _expr_val = (expr); \
    if(!_expr_val) { \
        OnCriticalError(err); \
    } \
})

#define ASSERT_TRUE(expr) ASSERT_TRUE_MSG(expr, "'" #expr "' failed")

static inline const std::string RootDirectory = "NitroEdit";
static inline const std::string ExternalFsDirectory = RootDirectory + "/ext_fs";