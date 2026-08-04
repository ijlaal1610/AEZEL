#pragma once
// ============================================================================
//  StorageManager — two-tier persistence.
//
//    NVS (Preferences)  -> small, high-write-frequency, wear-leveled data:
//                           odometer, trip counters, settings, calibration.
//    SD card             -> bulk data: full ride logs, CSV/GPX export,
//                           crash-log dumps, OTA staging, maintenance history.
//
//  Odometer/trip are written to NVS at most once every N seconds (see
//  FLUSH_INTERVAL_MS) rather than every tick — flash has finite write
//  cycles, and correctness only requires "no more than N seconds of data
//  lost on an abrupt power loss," which the buck converter's hold-up
//  capacitor plus PowerManager's linger window already covers.
// ============================================================================
#include <Arduino.h>
#include <Preferences.h>
#include <FS.h>
#include "DataModel.h"

struct RideLogPoint {
    uint32_t timestamp;
    float lat, lon, speedKmh;
    uint16_t rpm;
    float leanAngle;
};

class StorageManager {
public:
    static StorageManager& instance() { static StorageManager s; return s; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    // NVS-backed persistent counters (RideManager calls these)
    void saveOdometer(float km);
    float loadOdometer();
    void saveTripA(float km);
    void saveTripB(float km);
    float loadTripA();
    float loadTripB();

    // Maintenance reminders (service/tyre/chain/insurance/PUC intervals)
    void saveMaintenanceRecord(const char* key, uint32_t dueOdometerKm, uint32_t dueEpochSec);

    // SD-card bulk logging
    bool sdAvailable() const { return _sdOk; }
    void logRidePoint(const RideLogPoint& pt);   // appends to today's .csv
    bool exportTripCsv(const char* path);
    bool exportTripGpx(const char* path);
    void appendCrashLog(const char* reason);

    void flushAll();   // called from PowerManager before deep sleep

private:
    StorageManager() = default;
    Preferences _prefs;
    bool _sdOk = false;
    File _rideLogFile;
    uint32_t _lastFlushMs = 0;
    static constexpr uint32_t FLUSH_INTERVAL_MS = 10000;

    float _pendingOdometer = -1, _pendingTripA = -1, _pendingTripB = -1;
};
