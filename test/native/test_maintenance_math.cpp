// ============================================================================
//  test_maintenance_math.cpp — native, hardware-free tests.
//
//  Build & run:
//    g++ -std=c++17 -I ../../include -o test_maintenance_math test_maintenance_math.cpp ../../src/MaintenanceMath.cpp
//    ./test_maintenance_math
// ============================================================================
#include "MaintenanceMath.h"
#include <cstdio>

static int g_pass = 0, g_fail = 0;

static void expectStatus(const char* name, MaintenanceStatus actual, MaintenanceStatus expected) {
    const char* names[] = { "OK", "UPCOMING", "OVERDUE" };
    bool ok = actual == expected;
    if (ok) { g_pass++; std::printf("  [PASS] %s (got %s)\n", name, names[(int)actual]); }
    else    { g_fail++; std::printf("  [FAIL] %s (got %s, expected %s)\n", name, names[(int)actual], names[(int)expected]); }
}

static void expectEq(const char* name, uint32_t actual, uint32_t expected) {
    bool ok = actual == expected;
    if (ok) { g_pass++; std::printf("  [PASS] %s (got %u)\n", name, actual); }
    else    { g_fail++; std::printf("  [FAIL] %s (got %u, expected %u)\n", name, actual, expected); }
}

int main() {
    using namespace MaintenanceMath;
    std::printf("=== MaintenanceMath native test suite ===\n\n");

    std::printf("distanceStatus:\n");
    expectStatus("well before due -> OK", distanceStatus(1000.0f, 3000, 300.0f), MaintenanceStatus::OK);
    expectStatus("within warn window -> UPCOMING", distanceStatus(2800.0f, 3000, 300.0f), MaintenanceStatus::UPCOMING);
    expectStatus("exactly at due -> OVERDUE", distanceStatus(3000.0f, 3000, 300.0f), MaintenanceStatus::OVERDUE);
    expectStatus("past due -> OVERDUE", distanceStatus(3500.0f, 3000, 300.0f), MaintenanceStatus::OVERDUE);
    expectStatus("dueKm=0 (unconfigured) -> always OK, never nags", distanceStatus(999999.0f, 0, 300.0f), MaintenanceStatus::OK);
    expectStatus("boundary: exactly at warn window edge -> UPCOMING", distanceStatus(2700.0f, 3000, 300.0f), MaintenanceStatus::UPCOMING);
    expectStatus("boundary: just outside warn window -> OK", distanceStatus(2699.0f, 3000, 300.0f), MaintenanceStatus::OK);

    std::printf("\ndateStatus:\n");
    uint32_t day = 86400;
    expectStatus("60 days out, 14-day window -> OK", dateStatus(0, 60 * day, 14 * day), MaintenanceStatus::OK);
    expectStatus("10 days out, 14-day window -> UPCOMING", dateStatus(0, 10 * day, 14 * day), MaintenanceStatus::UPCOMING);
    expectStatus("expired yesterday -> OVERDUE", dateStatus(10 * day, 9 * day, 14 * day), MaintenanceStatus::OVERDUE);
    expectStatus("dueEpochSec=0 (unconfigured) -> always OK", dateStatus(999999999u, 0, 14 * day), MaintenanceStatus::OK);

    std::printf("\nnextDueOdometer:\n");
    expectEq("3000km service interval from 4521km", nextDueOdometer(4521.3f, 3000), 7521);
    expectEq("truncates fractional current km", nextDueOdometer(999.9f, 500), 1499);

    std::printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
