#pragma once
#include <cstdio>

inline int g_fails = 0;
inline void check(char const* name, bool ok) {
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    g_fails += !ok;
}
