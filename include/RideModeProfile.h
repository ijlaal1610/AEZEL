#pragma once
// ============================================================================
//  RideModeProfile.h — what each RideMode actually changes.
//
//  Deliberately a pure lookup table with zero hardware dependency (same
//  philosophy as VehicleMath.h) — the values themselves are the kind of
//  thing worth getting right on a desktop before they ever touch a
//  display or a warning threshold, and test/native/ covers this file too.
//
//  Per the original spec: "Each mode should modify: Display theme,
//  Brightness, Warnings, Data priority, Logging behaviour." This table is
//  where that mapping actually lives — SensorManager, RideManager, and
//  DisplayManager all read from here rather than each hard-coding their
//  own idea of what "Sport mode" means.
// ============================================================================
#include <cstdint>
#include "VehicleEnums.h"

struct RideModeProfile {
    ThemeMode theme;                    // display theme this mode selects
    uint8_t maxBrightness;                // backlight PWM ceiling (0-255)
    float fuelLowThresholdPct;             // below this %, raise FUEL_LOW
    float engineOvertempThresholdC;         // above this, raise ENGINE_OVERTEMP
    uint32_t rideLogIntervalMs;              // how often RideManager writes a GPS/ride-log point
};

namespace RideModeProfiles {

// Returns the profile for a given mode. Implemented as a plain switch over
// a static table rather than array-indexing RideMode directly, so adding a
// new enumerator that isn't yet in the table fails loudly (falls through to
// CITY's profile with a comment) instead of reading garbage.
const RideModeProfile& get(RideMode mode);

}  // namespace RideModeProfiles
