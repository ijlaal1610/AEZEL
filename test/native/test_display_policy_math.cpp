// ============================================================================
//  test_display_policy_math.cpp — native, hardware-free tests.
//
//  Build & run:
//    g++ -std=c++17 -I ../../include -o test_display_policy_math test_display_policy_math.cpp ../../src/DisplayPolicyMath.cpp
//    ./test_display_policy_math
// ============================================================================
#include "DisplayPolicyMath.h"
#include <cstdio>

static int g_pass = 0, g_fail = 0;

static void expectTrue(const char* name, bool cond) {
    if (cond) { g_pass++; std::printf("  [PASS] %s\n", name); }
    else      { g_fail++; std::printf("  [FAIL] %s\n", name); }
}

int main() {
    using namespace DisplayPolicyMath;
    std::printf("=== DisplayPolicyMath native test suite ===\n\n");

    std::printf("shouldSuppressBanner:\n");
    expectTrue("CRITICAL never suppressed even at high speed", !shouldSuppressBanner(true, 120.0f, 20.0f));
    expectTrue("CRITICAL never suppressed even parked", !shouldSuppressBanner(true, 0.0f, 20.0f));
    expectTrue("non-critical suppressed above threshold", shouldSuppressBanner(false, 30.0f, 20.0f));
    expectTrue("non-critical NOT suppressed below threshold", !shouldSuppressBanner(false, 10.0f, 20.0f));
    expectTrue("non-critical NOT suppressed exactly at threshold (strictly greater-than)", !shouldSuppressBanner(false, 20.0f, 20.0f));
    expectTrue("non-critical NOT suppressed while stationary", !shouldSuppressBanner(false, 0.0f, 20.0f));

    std::printf("\nisDataFresh:\n");
    expectTrue("just updated -> fresh", isDataFresh(1000, 1000, 15000));
    expectTrue("updated 5s ago, 15s window -> fresh", isDataFresh(1000, 6000, 15000));
    expectTrue("updated exactly at window edge -> fresh (inclusive)", isDataFresh(1000, 16000, 15000));
    expectTrue("updated just past window -> stale", !isDataFresh(1000, 16001, 15000));
    expectTrue("lastUpdateMs=0 (never received) -> always stale", !isDataFresh(0, 999999, 15000));
    expectTrue("now before lastUpdate (clock reset) -> stale, no underflow crash", !isDataFresh(50000, 100, 15000));

    std::printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
