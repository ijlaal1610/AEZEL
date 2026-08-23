#pragma once
// ============================================================================
//  BleManager — exposes a GATT service the companion mobile app subscribes
//  to for live telemetry (speed/RPM/fuel/warnings) and writes to for remote
//  commands (trip reset, ride-mode change, find-my-bike buzzer).
//
//  Kept minimal & versioned so the companion app and firmware can evolve
//  independently — payload is JSON over a single notify characteristic
//  rather than a field-per-characteristic design, trading a little
//  bandwidth for much easier future expansion (new dashboard fields don't
//  require a firmware+app GATT re-sync).
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

class BleManager {
public:
    static BleManager& instance() { static BleManager b; return b; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

private:
    BleManager() = default;
    void publishTelemetry();
    void handleIncomingCommand(const String& json);

    bool _started = false;
};
