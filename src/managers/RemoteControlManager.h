#pragma once
// ============================================================================
//  RemoteControlManager — the ONLY place phone commands are allowed to touch
//  an actuator. BleManager parses JSON and calls into here; it never writes
//  a GPIO directly. Centralizing this is what makes the interlock rules
//  auditable in one file instead of scattered through the BLE parser.
//
//  Read docs/remote_control.md before wiring ANY of these outputs — each
//  command below documents the interlock it enforces and why, but the
//  physical relay/wiring requirements live in that doc, not here.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

enum class RemoteResult : uint8_t { OK, REJECTED_MOVING, REJECTED_ENGINE_RUNNING,
                                     REJECTED_NOT_NEUTRAL, REJECTED_SIDESTAND_DOWN,
                                     REJECTED_ALREADY_ACTIVE, REJECTED_DISABLED, REJECTED_KILL_SWITCH };

class RemoteControlManager {
public:
    static RemoteControlManager& instance() { static RemoteControlManager r; return r; }

    void begin();
    static void taskEntry(void* pv);
    void tick();   // continuously re-validates interlocks while remote-start is active

    // Each returns a RemoteResult so BleManager can report back to the app
    // *why* something was refused rather than silently doing nothing.
    RemoteResult horn(bool on);
    RemoteResult hazard(bool on);
    RemoteResult indicator(bool left, bool on);
    RemoteResult setLocked(bool lock);          // immobilizer
    RemoteResult remoteStart();                  // begins remote-start sequence
    RemoteResult remoteStop();                   // always allowed, cuts starter+ignition-enable

    const char* lastResultString() const;

private:
    RemoteControlManager() = default;
    bool stationaryInterlockOk() const;   // speed==0 (indicators/horn/lock use this)
    void cutRemoteStart(const char* reason);

    RemoteResult _lastResult = RemoteResult::OK;
    bool _remoteStartActive = false;
    uint32_t _remoteStartBeganMs = 0;
    static constexpr uint32_t MAX_UNATTENDED_RUN_MS = 5 * 60 * 1000;   // auto-stop after 5 min
};
