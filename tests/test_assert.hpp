#pragma once

#include <iostream>
#include <string>

inline int expect_true(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        std::cerr << "FAIL " << file << ':' << line << " " << expr << '\n';
        return 1;
    }
    return 0;
}

#define EXPECT_TRUE(expr) \
    failures += expect_true(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

#define EXPECT_EQ(a, b) \
    failures += expect_true((a) == (b), #a " == " #b, __FILE__, __LINE__)
