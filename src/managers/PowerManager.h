#pragma once
// ============================================================================
//  PowerManager — ignition-triggered lifecycle, sleep states, safe shutdown.
//
//  Design principle: the ESP32 must NEVER be powered directly off ignition
//  (the bike's electrical noise/dropout on cranking would brown it out mid-
//  write and corrupt NVS/SD data). Power comes from a buck converter tied to
//  battery-positive (always-on), and PIN_IGNITION_SENSE just tells firmware
//  whether the rider has the key on — that's a *logical* signal, not power.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

enum class PowerState : uint8_t { BOOTING, ACTIVE, IGNITION_OFF_IDLE, DEEP_SLEEP, SHUTTING_DOWN };

class PowerManager {
public:
    static PowerManager& instance() { static PowerManager p; return p; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    PowerState state() const { return _state; }
    void requestSafeShutdown();   // called by SafetyMonitor or low-battery cutoff

private:
    PowerManager() = default;
    void enterDeepSleep();
    void handleIgnitionEdge(bool ignitionOn);

    PowerState _state = PowerState::BOOTING;
    uint32_t _ignitionOffSinceMs = 0;
    bool _lastIgnitionState = false;

    // How long to stay awake after ignition-off before deep sleep — lets the
    // rider see trip summary / lock animation / allows BLE sync to finish.
    static constexpr uint32_t IDLE_LINGER_MS = 15000;
};
