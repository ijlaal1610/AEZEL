#pragma once
// ============================================================================
//  SecurityMath.h — pure "has the bike moved" detection logic. Zero
//  hardware dependency, same philosophy as VehicleMath.h/MaintenanceMath.h.
//  A false trigger here means a 3am alarm that wakes the neighborhood for
//  nothing, or worse, a missed real theft because the threshold was tuned
//  wrong — worth getting the math right on a desktop first.
// ============================================================================
#include <cmath>

namespace SecurityMath {

// Approximate great-circle distance between two lat/lon points, in meters,
// using an equirectangular projection. This is NOT accurate over long
// distances (that's what haversine/Vincenty are for) but is more than
// precise enough for "did the bike move 15 meters" over the sub-kilometer
// scale a parked-bike drift check cares about, and is much cheaper to
// compute repeatedly on an ESP32 than trig-heavy haversine.
float gpsDriftMeters(double baseLat, double baseLon, double curLat, double curLon);

// Simple absolute deviation check for lean angle (degrees). Not a compass
// heading, so no wraparound handling needed — lean angle is bounded
// roughly -90..90.
bool angleDeviationExceeds(float baselineDeg, float currentDeg, float thresholdDeg);

}  // namespace SecurityMath
