// ============================================================================
//  test_vehicle_math.cpp — native, hardware-free tests.
//
//  Build & run (no PlatformIO, no ESP32 toolchain required):
//    g++ -std=c++17 -I ../../include -o test_vehicle_math test_vehicle_math.cpp ../../src/VehicleMath.cpp
//    ./test_vehicle_math
//
//  Or via PlatformIO's native test runner (also hardware-free):
//    pio test -e native
// ============================================================================
#include "VehicleMath.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>

static int g_pass = 0, g_fail = 0;

static void expectNear(const char* name, float actual, float expected, float tolerance) {
    bool ok = std::fabs(actual - expected) <= tolerance;
    if (ok) { g_pass++; std::printf("  [PASS] %s (got %.4f, expected %.4f +/- %.4f)\n", name, actual, expected, tolerance); }
    else    { g_fail++; std::printf("  [FAIL] %s (got %.4f, expected %.4f +/- %.4f)\n", name, actual, expected, tolerance); }
}

int main() {
    using namespace VehicleMath;
    std::printf("=== VehicleMath native test suite ===\n\n");

    // --- pulsesToKmh ------------------------------------------------------
    // 1 pulse/rev, 1.518m circumference wheel, 10 pulses in exactly 1 second
    // => 10 rev/s * 1.518 m/rev = 15.18 m/s = 54.65 km/h
    std::printf("pulsesToKmh:\n");
    expectNear("10 pulses/sec @ 1 pulse/rev", pulsesToKmh(10, 1, 1.518f, 1.0f), 54.648f, 0.05f);
    expectNear("0 pulses -> 0 km/h", pulsesToKmh(0, 1, 1.518f, 1.0f), 0.0f, 0.001f);
    expectNear("zero dt guarded (no div/0 crash)", pulsesToKmh(10, 1, 1.518f, 0.0f), 0.0f, 0.001f);

    // --- pulsesToRpm --------------------------------------------------
    // 1 pulse/rev, 100 pulses in 1 second -> 100 rev/s * 60 = 6000 RPM
    std::printf("\npulsesToRpm:\n");
    expectNear("100 pulses/sec -> 6000 RPM", pulsesToRpm(100, 1, 1.0f), 6000.0f, 0.5f);
    expectNear("idle: 5 pulses/sec -> 300 RPM", pulsesToRpm(5, 1, 1.0f), 300.0f, 0.5f);

    // --- emaUpdate ------------------------------------------------------
    std::printf("\nemaUpdate:\n");
    expectNear("alpha=1 snaps instantly to sample", emaUpdate(0.0f, 50.0f, 1.0f), 50.0f, 0.001f);
    expectNear("alpha=0 never moves from previous", emaUpdate(20.0f, 50.0f, 0.0f), 20.0f, 0.001f);
    expectNear("alpha=0.5 halves the gap", emaUpdate(0.0f, 100.0f, 0.5f), 50.0f, 0.001f);
    expectNear("alpha clamps above 1.0", emaUpdate(0.0f, 50.0f, 5.0f), 50.0f, 0.001f);

    // --- dividerVoltage ---------------------------------------------------
    // 3300mV ADC reading through a 5:1 divider -> 16.5V real voltage
    std::printf("\ndividerVoltage:\n");
    expectNear("3300mV @ 5x divider -> 16.5V", dividerVoltage(3300.0f, 5.0f), 16.5f, 0.01f);
    expectNear("0mV -> 0V", dividerVoltage(0.0f, 5.0f), 0.0f, 0.01f);

    // --- fuelRangeKm / kmPerLiter ------------------------------------
    std::printf("\nfuelRangeKm / kmPerLiter:\n");
    // 50% fuel, 0.4 km-per-percent estimate -> 20 km range
    expectNear("50%% fuel @ 0.4 km/%% -> 20km range", fuelRangeKm(50.0f, 0.4f), 20.0f, 0.01f);
    expectNear("0%% fuel -> 0km range", fuelRangeKm(0.0f, 0.4f), 0.0f, 0.01f);
    // 9L tank, 0.4 km/% (=> 40km per 100% => 40km per 9L => ~4.44 km/L)
    expectNear("9L tank @ 0.4 km/%% -> ~4.44 km/L", kmPerLiter(0.4f, 9.0f), 4.444f, 0.05f);

    // --- recalibrateKmPerPercent -----------------------------------
    std::printf("\nrecalibrateKmPerPercent:\n");
    // Covered 12km while fuel dropped 3% -> raw sample = 4.0 km/%
    // starting estimate 0.35, smoothing 0.3 -> 0.35*0.7 + 4.0*0.3 = 1.445
    expectNear("converges toward observed sample", recalibrateKmPerPercent(0.35f, 12.0f, 3.0f, 0.3f), 1.445f, 0.01f);
    expectNear("no fuel drop -> estimate unchanged", recalibrateKmPerPercent(0.35f, 12.0f, 0.0f, 0.3f), 0.35f, 0.001f);

    // --- speedToMeters ----------------------------------------------------
    std::printf("\nspeedToMeters:\n");
    // 36 km/h = 10 m/s, over 2 seconds = 20 meters
    expectNear("36km/h for 2s -> 20m", speedToMeters(36.0f, 2.0f), 20.0f, 0.01f);
    expectNear("0 km/h -> 0m", speedToMeters(0.0f, 5.0f), 0.0f, 0.01f);

    // --- Regression scenario: a full "tick" of realistic riding -----
    // Simulates 200ms-window pulse accumulation (matches SensorManager's
    // actual SPEED_RPM_CALC_WINDOW_MS) over 2.5 seconds of steady 40 km/h
    // riding and checks the accumulated distance matches simple physics.
    // This test is what originally caught a real firmware bug: computing
    // speed every 50ms tick with a 1-pulse-per-rev sensor undercounts to
    // zero at normal speeds. SensorManager now accumulates over a wider
    // 200ms window specifically because of what this test found.
    std::printf("\nRegression: 2.5s @ steady speed via pulse simulation:\n");
    {
        float wheelCirc = 1.518f;
        float windowSec = 0.2f;   // matches SPEED_RPM_CALC_WINDOW_MS
        uint8_t pulsesPerRev = 4;  // matches HALL_PULSES_PER_REV in Config.h
        float targetKmh = 40.0f;
        float targetMps = targetKmh / 3.6f;
        // pulses/window needed to sustain that speed with a 4-magnet wheel
        float revsPerWindow = (targetMps * windowSec) / wheelCirc;
        float pulsesPerWindow = revsPerWindow * pulsesPerRev;

        float speedEma = 0;
        float totalMeters = 0;
        int windows = (int)(2.5f / windowSec);
        for (int i = 0; i < windows; i++) {
            uint32_t pulses = (uint32_t)std::lround(pulsesPerWindow);
            float instKmh = pulsesToKmh(pulses, pulsesPerRev, wheelCirc, windowSec);
            speedEma = emaUpdate(speedEma, instKmh, 0.35f);
            totalMeters += speedToMeters(speedEma, windowSec);
        }
        expectNear("EMA converges near steady 40km/h after 2.5s", speedEma, targetKmh, 1.5f);
        float expectedMeters = targetMps * (windows * windowSec);
        // Wider tolerance here: EMA warm-up lag means the first ~1s
        // under-reports speed by design (that's the whole point of
        // smoothing away sensor noise), so total accumulated distance
        // legitimately comes in under the naive constant-speed estimate.
        expectNear("accumulated distance in right ballpark", totalMeters, expectedMeters, expectedMeters * 0.2f);
    }

    std::printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
