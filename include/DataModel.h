#pragma once
// ============================================================================
//  DataModel.h — Single source of truth for live vehicle state.
//
//  Every manager writes its own fields and reads others' via the mutex-guarded
//  accessors below. This avoids the "everyone pokes everyone else's globals"
//  trap and keeps the architecture swappable.
// ============================================================================
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "VehicleProfile.h"

enum class GearState : uint8_t { NEUTRAL, GEAR_1, GEAR_2, GEAR_3, GEAR_4, GEAR_5, GEAR_6, PARK, REVERSE, DRIVE, UNKNOWN };
enum class RideMode  : uint8_t { ECO, CITY, TOURING, SPORT, RAIN, CUSTOM };
enum class ThemeMode : uint8_t { LIGHT, DARK, CLASSIC_ANALOG, MODERN_DIGITAL, MINIMAL, SPORT, RETRO, NEON, CYBERPUNK, CUSTOM };

enum class WarningFlag : uint32_t {
    NONE                = 0,
    CHECK_ENGINE        = 1 << 0,
    OIL_PRESSURE        = 1 << 1,
    ENGINE_OVERTEMP     = 1 << 2,
    BATTERY_LOW         = 1 << 3,
    CHARGING_FAULT      = 1 << 4,
    FUEL_LOW            = 1 << 5,
    ABS_FAULT           = 1 << 6,
    SERVICE_DUE         = 1 << 7,
    TYRE_DUE            = 1 << 8,
    CHAIN_LUBE_DUE      = 1 << 9,
    INSURANCE_EXPIRING  = 1 << 10,
    PUC_EXPIRING        = 1 << 11,
    CRASH_DETECTED      = 1 << 12,
    UNAUTHORIZED_MOVE   = 1 << 13,
    GPS_LOST            = 1 << 14,
    SD_CARD_FAULT       = 1 << 15,
};
inline WarningFlag operator|(WarningFlag a, WarningFlag b) { return WarningFlag(uint32_t(a) | uint32_t(b)); }

inline const char* gearToString(GearState g) {
    switch (g) {
        case GearState::NEUTRAL: return "N";
        case GearState::GEAR_1:  return "1";
        case GearState::GEAR_2:  return "2";
        case GearState::GEAR_3:  return "3";
        case GearState::GEAR_4:  return "4";
        case GearState::GEAR_5:  return "5";
        case GearState::GEAR_6:  return "6";
        case GearState::PARK:    return "P";
        case GearState::REVERSE: return "R";
        case GearState::DRIVE:   return "D";
        default:                 return "-";
    }
}

struct VehicleState {
    // --- Active Vehicle Profile & Platform -----------------------------
    VehicleType      vehicleType       = VehicleType::BIKE;
    DataSourceMode   dataSource        = DataSourceMode::DIRECT_SENSOR;
    TransmissionType transmission      = TransmissionType::MANUAL;
    char             vehicleName[24]   = "AEZEL Cockpit";

    // --- Security & Lockscreen -----------------------------------------
    bool     enableLockscreen  = true;  // Enable/Disable Anti-Theft Lockscreen in Settings
    bool     isLocked          = false; // Anti-Theft Lockscreen currently active
    char     pinCode[5]        = "1234";

    // --- Display Toggles & Customization ------------------------------
    bool     showSpeedometer   = true;  // Option to hide center numeric speedo
    bool     focusMode         = false; // Minimalist Pure Tachometer mode
    bool     allowNotifOverlay = true;  // Popup live notifications on main display while riding

    // --- Core ride data -----------------------------------------------
    float    speedKmh          = 0;
    float    gpsSpeedKmh       = 0;
    uint16_t rpm                = 0;
    GearState gear              = GearState::UNKNOWN;
    char     gearDisplay[8]    = "N";   // Human-readable gear ("N", "1", "P", "D", etc.)
    float    tripA_km           = 0;
    float    tripB_km           = 0;
    float    odometer_km        = 0;
    uint32_t rideTimerSec        = 0;
    float    avgSpeedKmh        = 0;
    float    maxSpeedKmh        = 0;

    // --- OBD-II Telemetry (Available on OBD/Hybrid modes) ---------------
    float    throttlePct       = 0;     // Throttle Position Sensor (0-100%)
    float    engineLoadPct     = 0;     // Calculated Engine Load (0-100%)
    float    coolantTempC      = 0;     // Engine Coolant Temperature
    bool     obdConnected      = false; // True when TWAI CAN bus is receiving ECU responses

    // --- Navigation / GPS ------------------------------------------------
    double   latitude = 0, longitude = 0;
    float    altitudeM = 0;
    float    headingDeg = 0;
    bool     gpsFixValid = false;
    uint8_t  gpsSatellites = 0;

    // --- Turn-by-Turn Navigation (BLE companion map integration) --------
    bool     navActive = false;
    uint32_t navDistanceMeters = 0;
    uint8_t  navTurnIcon = 0;          // 0=Straight, 1=Left, 2=Right, 3=UTurn
    char     navStreetName[32] = "";
    uint16_t navEtaMinutes = 0;

    // --- Environment -----------------------------------------------------
    float    outsideTempC = 0;
    float    engineTempC  = 0;
    float    humidityPct  = 0;
    float    pressureHPa  = 0;
    float    lightLux     = 0;

    // --- Electrical ------------------------------------------------------
    float    batteryVoltage = 0;
    float    chargingVoltage = 0;
    float    currentDrawA   = 0;

    // --- Fuel --------------------------------------------------------
    float    fuelLevelPct       = 0;
    float    fuelRangeKm        = 0;
    float    fuelConsumptionKmL = 0;

    // --- IMU / dynamics ----------------------------------------------
    float    leanAngleDeg  = 0;
    float    pitchDeg      = 0;
    bool     crashSuspected = false;
    bool     fallDetected   = false;

    // --- Discrete indicator inputs (debounced, active = true) --------
    bool inLeftIndicator = false, inRightIndicator = false, inHazard = false;
    bool inNeutral = false, inHighBeam = false, inLowBeam = true;
    bool inSideStand = false, inKillSwitch = false, inIgnitionOn = false;
    bool inStarterActive = false, inEngineRunning = false;
    bool inFrontBrake = false, inRearBrake = false, inClutch = false;

    // --- System / mode -------------------------------------------------
    RideMode  rideMode  = RideMode::CITY;
    ThemeMode theme     = ThemeMode::MODERN_DIGITAL;
    uint32_t  activeWarnings = 0;   // bitmask of WarningFlag

    // --- Diagnostics -----------------------------------------------------
    uint32_t freeHeapBytes = 0;
    float    cpuLoadPct    = 0;
    bool     wifiConnected = false;
    bool     bleConnected  = false;
    bool     sdCardOk      = false;
    bool     gpsModuleOk   = false;
};

// ---------------------------------------------------------------------------
// Thread-safe accessor. All managers go through this.
// ---------------------------------------------------------------------------
class SharedState {
public:
    static SharedState& instance() {
        static SharedState s;
        return s;
    }

    VehicleState snapshot() const {
        VehicleState copy;
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            copy = _state;
            xSemaphoreGive(_mutex);
        } else {
            // Return latest cached state rather than all-zeros
            copy = _state;
        }
        return copy;
    }

    template <typename Fn>
    void update(Fn&& fn) {
        if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            fn(_state);
            xSemaphoreGive(_mutex);
        }
    }

    void raiseWarning(WarningFlag f)  { update([&](VehicleState& s) { s.activeWarnings |= uint32_t(f); }); }
    void clearWarning(WarningFlag f)  { update([&](VehicleState& s) { s.activeWarnings &= ~uint32_t(f); }); }
    bool hasWarning(WarningFlag f) const { return (snapshot().activeWarnings & uint32_t(f)) != 0; }

private:
    SharedState() { _mutex = xSemaphoreCreateMutex(); }
    VehicleState _state;
    mutable SemaphoreHandle_t _mutex = nullptr;
};
