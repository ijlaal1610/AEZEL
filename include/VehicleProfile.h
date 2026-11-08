#pragma once
// ============================================================================
//  VehicleProfile.h — Multi-Vehicle Configuration & Data Source Definition
//  Supports: Motorcycles (Bikes), Scooters (CVT), and Cars (OBD-II/Direct)
// ============================================================================
#include <stdint.h>
#include <stddef.h>

enum class VehicleType : uint8_t {
    BIKE = 0,       // Motorcycle (Manual transmission, lean angle, side stand)
    SCOOTER = 1,    // Scooter (CVT automatic, no gear indicator, belt drive)
    CAR = 2         // Automobile (4-wheel, no lean angle, no side stand, OBD/P-R-N-D)
};

enum class DataSourceMode : uint8_t {
    DIRECT_SENSOR = 0,  // Non-OBD: Discrete hall effect, coil pickup, analog float, DS18B20
    OBD_ONLY = 1,       // OBD-II via TWAI / CAN Bus: PIDs for RPM, speed, coolant, throttle, load
    HYBRID = 2          // Hybrid: OBD-II for powertrain + Direct Sensors for IMU, GPS, ambient
};

enum class TransmissionType : uint8_t {
    MANUAL = 0,         // Gears 1-N-2-3-4-5-6 with clutch
    CVT = 1,            // Continuously Variable Transmission (automatic scooter)
    AUTOMATIC = 2       // Car automatic: P-R-N-D
};

struct VehicleProfile {
    char                name[32];           // Friendly profile name (e.g. "Avenger 150", "Maruti Swift")
    VehicleType         vehicleType;        // BIKE, SCOOTER, or CAR
    DataSourceMode      dataSource;         // DIRECT_SENSOR, OBD_ONLY, or HYBRID
    TransmissionType    transmission;       // MANUAL, CVT, AUTOMATIC

    // Gauge limits & scales
    uint16_t            tachoMaxRpm;        // Full scale of tachometer arc (e.g. 12000, 9000, 7000)
    uint16_t            redlineRpm;         // Redline warning threshold (e.g. 9500, 7500, 5500)
    uint16_t            speedoMaxKmh;       // Full scale of speedometer (e.g. 180, 120, 220)
    uint8_t             maxGear;            // Top gear number (5 or 6 for bike/car, 0 for CVT)

    // Hardware & sensing constants
    float               wheelCircumferenceM;// Tyre roll distance in meters (e.g. 1.518m bike, 1.98m car)
    uint8_t             hallPulsesPerRev;   // Magnets on wheel for direct speed hall sensor
    uint8_t             rpmPulsesPerRev;    // Pulses per crankshaft revolution (opto/coil-negative)
    float               fuelTankCapacityL;  // Total tank capacity in liters
    float               batteryDividerRatio;// Voltage divider ratio (e.g. 5.0)

    // Alarm & threshold parameters
    float               engineOvertempC;    // Overtemperature warning threshold
    float               fuelLowPct;         // Low fuel alert threshold percentage
    float               batteryLowV;        // Low battery voltage threshold
    float               batteryHighV;       // Overcharging voltage threshold

    // Dynamic feature toggles per vehicle class
    bool                hasLeanAngle;       // Show lean angle & track cornering (bikes/scooters)
    bool                hasSideStand;       // Read & warn on side-stand switch (bikes/scooters)
    bool                hasClutch;          // Read clutch lever switch (manual transmission)
    bool                hasGearIndicator;   // Display gear number (disabled for CVT scooters)
    bool                hasTiltAlarm;       // Anti-theft tilt trigger (bikes/scooters) vs G-force (cars)
    bool                hasThrottleBar;     // Display real-time throttle position (available via OBD)
    bool                hasEngineLoadBar;   // Display real-time engine load (available via OBD)

    // Maintenance intervals (in km)
    uint32_t            serviceIntervalKm;  // General scheduled maintenance interval
    uint32_t            driveWearIntervalKm;// Chain lube (bikes) or drive belt check (scooters)
};

// String conversion helpers
inline const char* vehicleTypeToString(VehicleType t) {
    switch (t) {
        case VehicleType::BIKE:    return "Bike";
        case VehicleType::SCOOTER: return "Scooter";
        case VehicleType::CAR:     return "Car";
        default:                   return "Unknown";
    }
}

inline const char* dataSourceModeToString(DataSourceMode m) {
    switch (m) {
        case DataSourceMode::DIRECT_SENSOR: return "Direct Sensors (Non-OBD)";
        case DataSourceMode::OBD_ONLY:      return "OBD-II (CAN Bus)";
        case DataSourceMode::HYBRID:        return "Hybrid (OBD + Sensors)";
        default:                            return "Unknown";
    }
}

inline const char* transmissionTypeToString(TransmissionType t) {
    switch (t) {
        case TransmissionType::MANUAL:    return "Manual";
        case TransmissionType::CVT:       return "CVT (Automatic)";
        case TransmissionType::AUTOMATIC: return "Automatic (P-R-N-D)";
        default:                          return "Unknown";
    }
}
