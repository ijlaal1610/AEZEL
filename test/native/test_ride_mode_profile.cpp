// ============================================================================
//  test_ride_mode_profile.cpp — native, hardware-free tests for the
//  RideModeProfile lookup table.
//
//  Build & run:
//    g++ -std=c++17 -I ../../include -o test_ride_mode_profile test_ride_mode_profile.cpp ../../src/RideModeProfile.cpp
//    ./test_ride_mode_profile
// ============================================================================
#include "RideModeProfile.h"
#include <cstdio>
#include <cmath>

static int g_pass = 0, g_fail = 0;

static void expectTrue(const char* name, bool cond) {
    if (cond) { g_pass++; std::printf("  [PASS] %s\n", name); }
    else      { g_fail++; std::printf("  [FAIL] %s\n", name); }
}

int main() {
    std::printf("=== RideModeProfile native test suite ===\n\n");

    // Every mode must resolve to a real profile, not silently fall through
    // to garbage — this is the main thing worth testing here, since the
    // table itself is just data.
    const RideMode allModes[] = { RideMode::ECO, RideMode::CITY, RideMode::TOURING,
                                    RideMode::SPORT, RideMode::RAIN, RideMode::CUSTOM };
    const char* modeNames[] = { "ECO", "CITY", "TOURING", "SPORT", "RAIN", "CUSTOM" };

    std::printf("Sanity: every mode returns a profile with plausible values:\n");
    for (int i = 0; i < 6; i++) {
        const RideModeProfile& p = RideModeProfiles::get(allModes[i]);
        char buf[96];
        std::snprintf(buf, sizeof(buf), "%s: brightness in [20,255]", modeNames[i]);
        expectTrue(buf, p.maxBrightness >= 20 && p.maxBrightness <= 255);
        std::snprintf(buf, sizeof(buf), "%s: fuel threshold in (0,100]", modeNames[i]);
        expectTrue(buf, p.fuelLowThresholdPct > 0 && p.fuelLowThresholdPct <= 100);
        std::snprintf(buf, sizeof(buf), "%s: overtemp threshold plausible (60-150C)", modeNames[i]);
        expectTrue(buf, p.engineOvertempThresholdC >= 60 && p.engineOvertempThresholdC <= 150);
        std::snprintf(buf, sizeof(buf), "%s: log interval plausible (100ms-60s)", modeNames[i]);
        expectTrue(buf, p.rideLogIntervalMs >= 100 && p.rideLogIntervalMs <= 60000);
    }

    // These specific relationships are the actual design intent from
    // RideModeProfile.cpp's comments — worth pinning down so a future edit
    // can't silently invert them (e.g. Sport ending up dimmer than Eco).
    std::printf("\nDesign intent — relationships between modes:\n");
    const RideModeProfile& eco = RideModeProfiles::get(RideMode::ECO);
    const RideModeProfile& sport = RideModeProfiles::get(RideMode::SPORT);
    const RideModeProfile& touring = RideModeProfiles::get(RideMode::TOURING);
    const RideModeProfile& city = RideModeProfiles::get(RideMode::CITY);
    const RideModeProfile& custom = RideModeProfiles::get(RideMode::CUSTOM);

    expectTrue("Sport is brighter than Eco (visibility at speed)", sport.maxBrightness > eco.maxBrightness);
    expectTrue("Sport has the tightest overtemp margin of all modes",
                sport.engineOvertempThresholdC < eco.engineOvertempThresholdC &&
                sport.engineOvertempThresholdC < touring.engineOvertempThresholdC &&
                sport.engineOvertempThresholdC < city.engineOvertempThresholdC);
    expectTrue("Sport logs more frequently than Eco (smaller interval = more frequent)",
                sport.rideLogIntervalMs < eco.rideLogIntervalMs);
    expectTrue("Touring logs more frequently than City (longer trips want denser tracks)",
                touring.rideLogIntervalMs < city.rideLogIntervalMs);
    expectTrue("Custom has no brightness cap (full slider range respected)", custom.maxBrightness == 255);

    // Defensive: an out-of-range/garbage enum value must not read
    // uninitialized memory — the `default:` case in get() should catch it.
    std::printf("\nOut-of-range input safety:\n");
    RideMode bogus = (RideMode)255;
    const RideModeProfile& fallback = RideModeProfiles::get(bogus);
    expectTrue("unknown mode falls back to a valid profile, not garbage",
                fallback.maxBrightness >= 20 && fallback.maxBrightness <= 255);

    std::printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
