
#pragma once
#include <ntr/ntr_Include.hpp>
#include <nds.h>
#include <fat.h>

#define ATTR_NORETURN __attribute__((noreturn))

void ATTR_NORETURN OnCriticalError(const std::string &err);

#define ASSERT_TRUE_MSG(expr, err) ({ \
    const auto _expr_val = (expr); \
    if(!_expr_val) { \
        OnCriticalError(err); \
    } \
})

#define ASSERT_TRUE(expr) ASSERT_TRUE_MSG(expr, "'" #expr "' failed")