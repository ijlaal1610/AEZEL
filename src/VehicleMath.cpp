#include "VehicleMath.h"

namespace VehicleMath {

float pulsesToKmh(uint32_t pulseCount, uint8_t pulsesPerRev,
                    float wheelCircumferenceM, float dtSec) {
    if (dtSec <= 0 || pulsesPerRev == 0) return 0.0f;
    float revsPerSec = pulseCount / (float)pulsesPerRev / dtSec;
    return revsPerSec * wheelCircumferenceM * 3.6f;   // m/s -> km/h
}

float pulsesToRpm(uint32_t pulseCount, uint8_t pulsesPerRev, float dtSec) {
    if (dtSec <= 0 || pulsesPerRev == 0) return 0.0f;
    return (pulseCount / (float)pulsesPerRev / dtSec) * 60.0f;
}

float emaUpdate(float previous, float sample, float alpha) {
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    return previous + alpha * (sample - previous);
}

float dividerVoltage(float adcMilliVolts, float dividerRatio) {
    return (adcMilliVolts / 1000.0f) * dividerRatio;
}

float fuelRangeKm(float fuelPct, float kmPerPercent) {
    if (fuelPct < 0) fuelPct = 0;
    return fuelPct * kmPerPercent;
}

float kmPerLiter(float kmPerPercent, float tankCapacityL) {
    if (kmPerPercent <= 0) return 0.0f;
    float litersPerKm = (tankCapacityL / 100.0f) / kmPerPercent;
    if (litersPerKm <= 0) return 0.0f;
    return 1.0f / litersPerKm;
}

float recalibrateKmPerPercent(float currentEstimate, float kmCoveredSinceCheckpoint,
                                float fuelPctDropSinceCheckpoint, float smoothingFactor) {
    if (fuelPctDropSinceCheckpoint <= 0) return currentEstimate;
    float sample = kmCoveredSinceCheckpoint / fuelPctDropSinceCheckpoint;
    if (smoothingFactor < 0.0f) smoothingFactor = 0.0f;
    if (smoothingFactor > 1.0f) smoothingFactor = 1.0f;
    return currentEstimate * (1.0f - smoothingFactor) + sample * smoothingFactor;
}

float speedToMeters(float speedKmh, float dtSec) {
    return (speedKmh / 3.6f) * dtSec;
}

}  // namespace VehicleMath
