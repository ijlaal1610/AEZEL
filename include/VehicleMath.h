#pragma once
// ============================================================================
//  VehicleMath.h — pure, hardware-free math used by SensorManager and
//  RideManager. Deliberately has ZERO Arduino/FreeRTOS/ESP32 dependency so
//  it can be compiled and unit-tested on a plain desktop compiler (see
//  test/native/) before you ever have a wheel or a fuel tank to test
//  against. This is the actual correctness-critical logic — the managers
//  themselves are mostly plumbing (reading pins, writing to SharedState)
//  around these functions.
// ============================================================================
#include <cstdint>

namespace VehicleMath {

// Converts an interrupt pulse count over a time window into speed (km/h),
// given the wheel's pulses-per-revolution and circumference.
float pulsesToKmh(uint32_t pulseCount, uint8_t pulsesPerRev,
                    float wheelCircumferenceM, float dtSec);

// Converts a pulse count over a time window into RPM.
float pulsesToRpm(uint32_t pulseCount, uint8_t pulsesPerRev, float dtSec);

// Exponential moving average — smooths noisy per-tick samples.
// alpha in (0,1]; higher = more responsive, lower = smoother/slower.
float emaUpdate(float previous, float sample, float alpha);

// Converts a raw ADC millivolt reading through a resistive divider into the
// real-world voltage upstream of the divider.
float dividerVoltage(float adcMilliVolts, float dividerRatio);

// Estimated remaining range in km given fuel percentage and the current
// km-per-percent calibration factor.
float fuelRangeKm(float fuelPct, float kmPerPercent);

// Converts km-per-percent into an approximate km/L figure given tank
// capacity in liters.
float kmPerLiter(float kmPerPercent, float tankCapacityL);

// Recalibrates the km-per-percent estimate using a smoothed update once a
// meaningful fuel-percentage drop has been observed since the last
// checkpoint. Returns the new estimate.
float recalibrateKmPerPercent(float currentEstimate, float kmCoveredSinceCheckpoint,
                                float fuelPctDropSinceCheckpoint, float smoothingFactor);

// Integrates instantaneous speed (km/h) over a timestep into meters
// traveled — the core of trip/odometer accumulation.
float speedToMeters(float speedKmh, float dtSec);

}  // namespace VehicleMath
