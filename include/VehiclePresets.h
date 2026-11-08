#pragma once
// ============================================================================
//  VehiclePresets.h — Built-in Default Profiles for Bikes, Scooters, and Cars
// ============================================================================
#include "VehicleProfile.h"

// ----------------------------------------------------------------------------
// 1. MOTORCYCLES (BIKES)
// ----------------------------------------------------------------------------

// Standard Non-OBD Motorcycle (e.g. Bajaj Avenger 150, Royal Enfield Classic 350, Pulsar 150)
// Uses direct wheel hall sensor, coil pickup, analog float, and discrete harness switches.
constexpr VehicleProfile PRESET_BIKE_DIRECT = {
    "Bike (Direct/Non-OBD)",
    VehicleType::BIKE,
    DataSourceMode::DIRECT_SENSOR,
    TransmissionType::MANUAL,
    12000,                  // tachoMaxRpm
    9500,                   // redlineRpm
    180,                    // speedoMaxKmh
    5,                      // maxGear (1-N-2-3-4-5)
    1.518f,                 // wheelCircumferenceM (90/90-18)
    1,                      // hallPulsesPerRev
    1,                      // rpmPulsesPerRev (single-cylinder waste spark / coil-negative)
    13.0f,                  // fuelTankCapacityL
    5.0f,                   // batteryDividerRatio
    120.0f,                 // engineOvertempC
    15.0f,                  // fuelLowPct
    11.8f,                  // batteryLowV
    14.8f,                  // batteryHighV
    true,                   // hasLeanAngle
    true,                   // hasSideStand
    true,                   // hasClutch
    true,                   // hasGearIndicator
    true,                   // hasTiltAlarm
    false,                  // hasThrottleBar (unavailable on non-OBD)
    false,                  // hasEngineLoadBar
    3000,                   // serviceIntervalKm
    500                     // chain lube interval (km)
};

// Modern OBD / EFI Motorcycle (e.g. KTM Duke 390, Bajaj Dominar 400, Yamaha MT-15)
// Hybrid mode: Engine RPM, Coolant Temp, Speed, Throttle from CAN Bus; IMU & GPS direct.
constexpr VehicleProfile PRESET_BIKE_OBD = {
    "Bike (OBD Hybrid)",
    VehicleType::BIKE,
    DataSourceMode::HYBRID,
    TransmissionType::MANUAL,
    14000,                  // tachoMaxRpm
    10500,                  // redlineRpm
    200,                    // speedoMaxKmh
    6,                      // maxGear (1-N-2-3-4-5-6)
    1.880f,                 // wheelCircumferenceM (150/60-17)
    4,                      // hallPulsesPerRev
    1,                      // rpmPulsesPerRev
    13.5f,                  // fuelTankCapacityL
    5.0f,                   // batteryDividerRatio
    110.0f,                 // engineOvertempC
    15.0f,                  // fuelLowPct
    11.8f,                  // batteryLowV
    14.8f,                  // batteryHighV
    true,                   // hasLeanAngle
    true,                   // hasSideStand
    true,                   // hasClutch
    true,                   // hasGearIndicator
    true,                   // hasTiltAlarm
    true,                   // hasThrottleBar (OBD PID 0x11)
    true,                   // hasEngineLoadBar (OBD PID 0x04)
    5000,                   // serviceIntervalKm
    500                     // chain lube interval (km)
};

// ----------------------------------------------------------------------------
// 2. SCOOTERS
// ----------------------------------------------------------------------------

// Standard Non-OBD Scooter (e.g. Honda Activa 110/125, TVS Jupiter ZX, Suzuki Access 125)
// CVT automatic (no clutch lever, no gear display), smaller 10-12" wheels, drive belt.
constexpr VehicleProfile PRESET_SCOOTER_DIRECT = {
    "Scooter (Direct/CVT)",
    VehicleType::SCOOTER,
    DataSourceMode::DIRECT_SENSOR,
    TransmissionType::CVT,
    9000,                   // tachoMaxRpm
    7500,                   // redlineRpm
    120,                    // speedoMaxKmh
    0,                      // maxGear (CVT - no gear display)
    1.340f,                 // wheelCircumferenceM (90/90-12 front / 90/100-10 rear)
    1,                      // hallPulsesPerRev
    1,                      // rpmPulsesPerRev
    5.3f,                   // fuelTankCapacityL
    5.0f,                   // batteryDividerRatio
    110.0f,                 // engineOvertempC
    20.0f,                  // fuelLowPct
    11.8f,                  // batteryLowV
    14.8f,                  // batteryHighV
    true,                   // hasLeanAngle
    true,                   // hasSideStand
    false,                  // hasClutch (CVT automatic centrifugal clutch)
    false,                  // hasGearIndicator (no gears)
    true,                   // hasTiltAlarm
    false,                  // hasThrottleBar
    false,                  // hasEngineLoadBar
    3000,                   // serviceIntervalKm
    8000                    // drive belt inspection interval (km)
};

// Connected / EFI Scooter (e.g. TVS Ntorq Race XP, Ather 450X, Ola S1)
constexpr VehicleProfile PRESET_SCOOTER_OBD = {
    "Scooter (OBD Hybrid)",
    VehicleType::SCOOTER,
    DataSourceMode::HYBRID,
    TransmissionType::CVT,
    10000,                  // tachoMaxRpm
    8500,                   // redlineRpm
    120,                    // speedoMaxKmh
    0,                      // maxGear
    1.380f,                 // wheelCircumferenceM (110/80-12)
    1,                      // hallPulsesPerRev
    1,                      // rpmPulsesPerRev
    5.8f,                   // fuelTankCapacityL
    5.0f,                   // batteryDividerRatio
    105.0f,                 // engineOvertempC
    15.0f,                  // fuelLowPct
    11.8f,                  // batteryLowV
    14.8f,                  // batteryHighV
    true,                   // hasLeanAngle
    true,                   // hasSideStand
    false,                  // hasClutch
    false,                  // hasGearIndicator
    true,                   // hasTiltAlarm
    true,                   // hasThrottleBar
    true,                   // hasEngineLoadBar
    4000,                   // serviceIntervalKm
    10000                   // drive belt replacement (km)
};

// ----------------------------------------------------------------------------
// 3. CARS (AUTOMOBILES)
// ----------------------------------------------------------------------------

// Standard Manual Car via OBD-II (e.g. Maruti Suzuki Swift, Hyundai i20, Honda City)
// Standard OBD-II powertrain telemetry. No lean angle, no side-stand.
constexpr VehicleProfile PRESET_CAR_OBD = {
    "Car Manual (OBD-II)",
    VehicleType::CAR,
    DataSourceMode::OBD_ONLY,
    TransmissionType::MANUAL,
    7000,                   // tachoMaxRpm
    6000,                   // redlineRpm
    220,                    // speedoMaxKmh
    5,                      // maxGear
    1.980f,                 // wheelCircumferenceM (185/65-R15)
    0,                      // hallPulsesPerRev (sourced from ECU PID 0x0D)
    0,                      // rpmPulsesPerRev (sourced from ECU PID 0x0C)
    37.0f,                  // fuelTankCapacityL
    5.0f,                   // batteryDividerRatio
    105.0f,                 // engineOvertempC
    12.0f,                  // fuelLowPct
    11.8f,                  // batteryLowV
    14.8f,                  // batteryHighV
    false,                  // hasLeanAngle (not applicable for cars)
    false,                  // hasSideStand
    true,                   // hasClutch (manual transmission)
    true,                   // hasGearIndicator
    false,                  // hasTiltAlarm (uses G-force crash instead)
    true,                   // hasThrottleBar
    true,                   // hasEngineLoadBar
    10000,                  // serviceIntervalKm
    20000                   // tyre rotation interval (km)
};

// Automatic Passenger Car via OBD-II (e.g. Hyundai Creta, Kia Seltos, Honda City CVT/AT)
// Displays P-R-N-D gear positions, torque load, coolant temp, throttle.
constexpr VehicleProfile PRESET_CAR_AUTO_OBD = {
    "Car Auto (OBD-II P-R-N-D)",
    VehicleType::CAR,
    DataSourceMode::OBD_ONLY,
    TransmissionType::AUTOMATIC,
    7000,                   // tachoMaxRpm
    6000,                   // redlineRpm
    240,                    // speedoMaxKmh
    0,                      // maxGear (Automatic uses P-R-N-D states)
    2.050f,                 // wheelCircumferenceM (205/60-R16)
    0,                      // hallPulsesPerRev
    0,                      // rpmPulsesPerRev
    50.0f,                  // fuelTankCapacityL
    5.0f,                   // batteryDividerRatio
    105.0f,                 // engineOvertempC
    10.0f,                  // fuelLowPct
    11.8f,                  // batteryLowV
    14.8f,                  // batteryHighV
    false,                  // hasLeanAngle
    false,                  // hasSideStand
    false,                  // hasClutch
    false,                  // hasGearIndicator (P-R-N-D used instead)
    false,                  // hasTiltAlarm
    true,                   // hasThrottleBar
    true,                   // hasEngineLoadBar
    10000,                  // serviceIntervalKm
    20000                   // tyre rotation / transmission fluid check (km)
};

// Preset catalog array for index-based selection
constexpr size_t TOTAL_PRESETS = 6;
constexpr VehicleProfile VEHICLE_PRESET_CATALOG[TOTAL_PRESETS] = {
    PRESET_BIKE_DIRECT,
    PRESET_BIKE_OBD,
    PRESET_SCOOTER_DIRECT,
    PRESET_SCOOTER_OBD,
    PRESET_CAR_OBD,
    PRESET_CAR_AUTO_OBD
};
