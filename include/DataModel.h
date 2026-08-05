#pragma once
// ============================================================================
//  DataModel.h — Single source of truth for live vehicle state.
//
//  Every manager writes its own fields and reads others' via the mutex-guarded
//  accessors below. This avoids the "everyone pokes everyone else's globals"
//  trap and keeps the architecture swappable (e.g. mock this whole struct in
//  unit tests without touching real hardware).
// ============================================================================
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum class GearState : uint8_t { NEUTRAL, GEAR_1, GEAR_2, GEAR_3, GEAR_4, GEAR_5, UNKNOWN };
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

struct VehicleState {
    // --- Display Toggles & Customization ------------------------------
    bool     showSpeedometer   = true;  // Option to hide center numeric speedo
    bool     focusMode         = false; // Minimalist Pure Tachometer mode

    // --- Core ride data -----------------------------------------------
    float    speedKmh          = 0;
    float    gpsSpeedKmh       = 0;
    uint16_t rpm                = 0;
    GearState gear              = GearState::UNKNOWN;
    float    tripA_km           = 0;
    float    tripB_km           = 0;
    float    odometer_km        = 0;
    uint32_t rideTimerSec        = 0;
    float    avgSpeedKmh        = 0;
    float    maxSpeedKmh        = 0;

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
// Thread-safe accessor. All managers go through this — never touch a raw
// VehicleState instance directly across task boundaries.
// ---------------------------------------------------------------------------
class SharedState {
public:
    static SharedState& instance() {
        static SharedState s;
        return s;
    }

    // Read a consistent snapshot (cheap struct copy under a short-held mutex)
    VehicleState snapshot() const {
        VehicleState copy;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            copy = _state;
            xSemaphoreGive(_mutex);
        }
        return copy;
    }

    // Mutate under lock via a lambda: SharedState::instance().update([](VehicleState&s){ s.rpm = 4200; });
    template <typename Fn>
    void update(Fn&& fn) {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
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
    mutable SemaphoreHandle_t _mutex;
};
