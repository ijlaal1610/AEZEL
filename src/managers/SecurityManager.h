#pragma once
// ============================================================================
//  SecurityManager — arms/disarms and continuously monitors for
//  unauthorized movement while parked. This is what closes the gap on
//  `WarningFlag::UNAUTHORIZED_MOVE` and the "Motion Alarm / Tilt Alarm /
//  Tow Detection" spec items — both existed as scaffolding (the flag, the
//  notification title) with nothing ever raising them.
//
//  Three independent triggers, layered by hardware tier so this is useful
//  starting from Tier 1 (see docs/incremental_build.md) and gets better as
//  more parts are added — none of them require ENABLE_SECURITY_ALARM's
//  baseline (buzzer + wheel sensor) to be extended, they're additive:
//    - Wheel motion  (Tier 1, always available): the bike being pushed,
//      rolled, or lifted-and-spun triggers this with zero extra hardware.
//    - Lean angle deviation (Tier 4, needs ENABLE_IMU): catches someone
//      tilting/lifting the bike without necessarily turning the wheel.
//    - GPS drift (Tier 3, needs ENABLE_GPS): catches a flatbed tow, where
//      the wheels never turn and the bike never tilts.
//
//  Alarm output (see docs/security.md): car-style — horn chirps in sync
//  with hazard-light blinks, repeating for SECURITY_ALARM_DURATION_MS.
//  Needs the Tier 1.5 horn/indicator relays (ENABLE_REMOTE_HORN /
//  ENABLE_REMOTE_INDICATORS). Without that hardware yet, falls back to
//  pulsing the onboard buzzer (Tier 0/1, no extra parts) so arming still
//  does *something* useful before you've bought the relays — but the
//  buzzer is a fallback, not the intended experience.
//
//  Honesty note (see docs/security.md): this is a local alarm + BLE
//  notification, not a cellular tracker. If the phone isn't in BLE range
//  when the alarm fires, the audible/visual deterrent still happens, but
//  the phone only learns about it on the next connection.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

enum class SecurityState : uint8_t { DISARMED, ARMED, ALARM_ACTIVE };
enum class SecurityResult : uint8_t { OK, REJECTED_MOVING, REJECTED_ENGINE_RUNNING, REJECTED_ALREADY_ARMED, REJECTED_NOT_ARMED, REJECTED_DISABLED };

class SecurityManager {
public:
    static SecurityManager& instance() { static SecurityManager s; return s; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    SecurityResult arm();       // captures baselines, only allowed stationary with engine off
    SecurityResult disarm();    // always allowed — stops any active alarm immediately

    SecurityState state() const { return _state; }
    const char* lastResultString() const;
    const char* stateString() const;

private:
    SecurityManager() = default;
    void evaluateTriggers();
    void triggerAlarm(const char* reason);
    void stopAlarmOutput();

    SecurityState _state = SecurityState::DISARMED;
    SecurityResult _lastResult = SecurityResult::OK;

    float _baselineLeanDeg = 0;
    double _baselineLat = 0, _baselineLon = 0;
    bool _baselineGpsValid = false;

    uint32_t _alarmStartedMs = 0;
    uint32_t _lastPulseToggleMs = 0;
    bool _pulseOn = false;   // drives horn+hazard together (or the buzzer fallback) each toggle
};
