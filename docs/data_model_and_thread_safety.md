# AEZEL — Data Model & Thread Safety Guide

This document details the central data architecture, thread synchronization model, and concurrency rules defined in [`include/DataModel.h`](file:///workspaces/AEZEL/include/DataModel.h).

---

## 📑 Table of Contents

- [1. Shared Memory Architecture](#1-shared-memory-architecture)
- [2. VehicleState Data Structure](#2-vehiclestate-data-structure)
- [3. SharedState Thread-Safe Wrapper](#3-sharedstate-thread-safe-wrapper)
- [4. Concurrency Rules & Deadlock Prevention](#4-concurrency-rules--deadlock-prevention)

---

## 1. Shared Memory Architecture

In multi-tasking systems running on dual-core processors, data corruption occurs if one core writes to a struct while another core reads it. AEZEL solves this by forcing all cross-task data exchange through a single, mutex-guarded singleton class: **`SharedState`**.

```
[ SensorManager ] ---\
[ RideManager   ] ----\                        +------------------------+
[ GpsManager    ] -----> (SharedState Mutex) -> | VehicleState Snapshot  |
[ PowerManager  ] ----/                        +------------------------+
                                                           |
                                           +---------------+---------------+
                                           |               |               |
                                     [ DisplayMgr ]   [ BleMgr ]   [ StorageMgr ]
                                       (Core 1)        (Core 0)       (Core 0)
```

---

## 2. VehicleState Data Structure

The `VehicleState` struct defines every telemetry field in the system:

```cpp
struct VehicleState {
    // Instrumentation
    float speedKmh = 0.0f;
    uint16_t rpm = 0;
    GearState gear = GearState::NEUTRAL;
    float fuelLevelPct = 100.0f;
    float engineTempC = 0.0f;
    float ambientTempC = 25.0f;
    
    // Electrical & Power
    float batteryVoltage = 12.6f;
    float chargeVoltage = 13.8f;
    PowerState powerState = PowerState::ACTIVE;
    
    // Distance & Trips
    double odometer_km = 0.0;
    float tripA_km = 0.0f;
    float tripB_km = 0.0f;
    uint32_t rideTimeSeconds = 0;
    
    // Motion & Orientation
    float leanAngleRoll = 0.0f;
    float pitchAngle = 0.0f;
    float baroAltitudeM = 0.0f;
    
    // Discrete Harness Inputs
    bool inLeftIndicator = false;
    bool inRightIndicator = false;
    bool inNeutral = false;
    bool inHighBeam = false;
    bool inSideStandDown = false;
    bool inBrakeActive = false;
    bool inKillSwitchOn = false;
    bool inIgnitionOn = true;
    
    // Warnings & Status Bitmask
    uint16_t activeWarnings = 0; // Bitmask of WarningFlag enum
    bool bleConnected = false;
    
    // GPS Coordinates
    double latitude = 0.0;
    double longitude = 0.0;
    float gpsAltitudeM = 0.0f;
    float gpsHeadingDeg = 0.0f;
    uint8_t gpsSatellites = 0;
};
```

---

## 3. SharedState Thread-Safe Wrapper

Guarded by a FreeRTOS `SemaphoreHandle_t _mutex`:

```cpp
class SharedState {
public:
    static SharedState& instance() {
        static SharedState s;
        return s;
    }

    // Atomic snapshot (Read-Only Copy)
    VehicleState snapshot() {
        VehicleState copy;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            copy = _state;
            xSemaphoreGive(_mutex);
        }
        return copy;
    }

    // Atomic update lambda (Write)
    template<typename Func>
    void update(Func&& modifier) {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            modifier(_state);
            xSemaphoreGive(_mutex);
        }
    }

private:
    SharedState() { _mutex = xSemaphoreCreateMutex(); }
    VehicleState _state;
    SemaphoreHandle_t _mutex;
};
```

---

## 4. Concurrency Rules & Deadlock Prevention

1. **Short Lock Times**: Mutex lock durations are limited to copy operations or simple field assignments (< 5 microseconds).
2. **No I/O Inside Lock**: Managers must **NEVER** perform blocking I/O (SD card writes, BLE broadcasts, display flushes) while holding the `SharedState` mutex lock.
3. **Copy-on-Read Pattern**: Consumers call `snapshot()` to get a stack-allocated copy of `VehicleState`, release the lock immediately, and then render or log the data independently.
