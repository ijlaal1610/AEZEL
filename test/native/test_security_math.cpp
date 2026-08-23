// ============================================================================
//  test_security_math.cpp — native, hardware-free tests.
//
//  Build & run:
//    g++ -std=c++17 -I ../../include -o test_security_math test_security_math.cpp ../../src/SecurityMath.cpp
//    ./test_security_math
// ============================================================================
#include "SecurityMath.h"
#include <cstdio>
#include <cmath>

static int g_pass = 0, g_fail = 0;

static void expectNear(const char* name, float actual, float expected, float tolerance) {
    bool ok = std::fabs(actual - expected) <= tolerance;
    if (ok) { g_pass++; std::printf("  [PASS] %s (got %.3f, expected %.3f +/- %.3f)\n", name, actual, expected, tolerance); }
    else    { g_fail++; std::printf("  [FAIL] %s (got %.3f, expected %.3f +/- %.3f)\n", name, actual, expected, tolerance); }
}

static void expectTrue(const char* name, bool cond) {
    if (cond) { g_pass++; std::printf("  [PASS] %s\n", name); }
    else      { g_fail++; std::printf("  [FAIL] %s\n", name); }
}

int main() {
    using namespace SecurityMath;
    std::printf("=== SecurityMath native test suite ===\n\n");

    std::printf("gpsDriftMeters:\n");
    expectNear("same point -> 0m", gpsDriftMeters(28.6139, 77.2090, 28.6139, 77.2090), 0.0f, 0.01f);
    // 1 degree of latitude is ~111,320m — a well-known reference figure for
    // sanity-checking any lat/lon distance formula.
    expectNear("1 degree latitude -> ~111.32km", gpsDriftMeters(0.0, 0.0, 1.0, 0.0), 111320.0f, 500.0f);
    // 0.0001 degree latitude ~= 11.1m — the scale that actually matters for
    // "did a parked bike get moved 15 meters."
    expectNear("0.0001 degree latitude -> ~11.1m", gpsDriftMeters(28.0, 77.0, 28.0001, 77.0), 11.1f, 1.0f);

    std::printf("\nangleDeviationExceeds:\n");
    expectTrue("10 degree swing vs 8 degree threshold -> triggers", angleDeviationExceeds(0.0f, 10.0f, 8.0f));
    expectTrue("2 degree swing vs 8 degree threshold -> does not trigger", !angleDeviationExceeds(0.0f, 2.0f, 8.0f));
    expectTrue("negative swing counts by magnitude, not sign", angleDeviationExceeds(0.0f, -10.0f, 8.0f));
    expectTrue("exactly at threshold does not trigger (strictly greater-than)", !angleDeviationExceeds(0.0f, 8.0f, 8.0f));
    expectTrue("baseline itself never triggers", !angleDeviationExceeds(15.0f, 15.0f, 8.0f));

    std::printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
