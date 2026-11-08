#pragma once
// ============================================================================
//  CanManager — Automotive CAN Bus & OBD-II Telemetry Subsystem
//
//  Interfaces with modern motorcycle ECU networks (ISO 11898-2 CAN 2.0B / OBD-II)
//  using the ESP32-S3's built-in Two-Wire Automotive Interface (TWAI) controller.
//  Polls standard OBD-II PIDs:
//    - PID 0x0C: Engine RPM
//    - PID 0x0D: Vehicle Speed
//    - PID 0x05: Engine Coolant Temperature
//    - PID 0x11: Throttle Position
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

class CanManager {
public:
    static CanManager& instance() { static CanManager c; return c; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    bool isCanActive() const { return _canActive; }

private:
    CanManager() = default;

    bool _canActive = false;
    uint32_t _lastPollMs = 0;
};
