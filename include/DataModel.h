#pragma once
// ============================================================================
//  DataModel.h — Single source of truth for live vehicle state.
//
//  Every manager writes its own fields and reads others' via the mutex-guarded
//  accessors below. This avoids the "everyone pokes everyone else's globals"
//  trap and keeps the architecture swappable (e.g. mock this whole struct in
//  unit tests without touching real hardware).
//
//  The enums used below (GearState/RideMode/ThemeMode/WarningFlag) live in
//  VehicleEnums.h, which has no hardware dependency — only the
//  VehicleState/SharedState machinery in THIS file needs Arduino/FreeRTOS,
//  for the mutex. Pure logic modules that just need the enums (e.g.
//  RideModeProfile.h) include VehicleEnums.h directly instead of this file.
// ============================================================================
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "VehicleEnums.h"

struct VehicleState {
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
