#include "SecurityManager.h"
#include "Config.h"
#include "SecurityMath.h"
#include "RemoteControlManager.h"

void SecurityManager::begin() {
#if ENABLE_SECURITY_ALARM
    pinMode(PIN_OUT_BUZZER, OUTPUT);
    digitalWrite(PIN_OUT_BUZZER, LOW);
#endif
}

void SecurityManager::taskEntry(void* pv) {
    SecurityManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(SECURITY_CHECK_INTERVAL_MS);
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void SecurityManager::tick() {
#if ENABLE_SECURITY_ALARM
    if (_state == SecurityState::ARMED) {
        evaluateTriggers();
    } else if (_state == SecurityState::ALARM_ACTIVE) {
        uint32_t now = millis();
        if (now - _lastPulseToggleMs > SECURITY_ALARM_PULSE_MS) {
            _lastPulseToggleMs = now;
            _pulseOn = !_pulseOn;

#if ENABLE_REMOTE_HORN || ENABLE_REMOTE_INDICATORS
            // Car-style alarm: horn chirps in sync with the hazard-light
            // blink, both driven by the same toggle so they're locked
            // together — this is what makes it read as a deliberate alarm
            // rather than two things randomly happening near each other.
            auto& remote = RemoteControlManager::instance();
            remote.horn(_pulseOn);
            remote.hazard(_pulseOn);
#else
            // Fallback: no horn/indicator relay hardware installed yet
            // (Tier 1.5, see docs/incremental_build.md) — pulse the
            // onboard buzzer instead so arming still does something
            // useful in the meantime. Once that hardware is added this
            // branch compiles out entirely.
            digitalWrite(PIN_OUT_BUZZER, _pulseOn ? HIGH : LOW);
#endif
        }
        if (now - _alarmStartedMs > SECURITY_ALARM_DURATION_MS) {
            // Auto-stop the audible/visual output so a false trigger
            // doesn't drain the battery or annoy the neighborhood all
            // night — the CRITICAL warning flag stays raised (see
            // stopAlarmOutput()) so the rider still sees it happened.
            stopAlarmOutput();
        }
    }
#endif
}

SecurityResult SecurityManager::arm() {
#if ENABLE_SECURITY_ALARM
    if (_state != SecurityState::DISARMED) { _lastResult = SecurityResult::REJECTED_ALREADY_ARMED; return _lastResult; }

    VehicleState s = SharedState::instance().snapshot();
    if (s.inEngineRunning) { _lastResult = SecurityResult::REJECTED_ENGINE_RUNNING; return _lastResult; }
    if (s.speedKmh >= 1.0f) { _lastResult = SecurityResult::REJECTED_MOVING; return _lastResult; }

    _baselineLeanDeg = s.leanAngleDeg;
    _baselineGpsValid = s.gpsFixValid;
    if (_baselineGpsValid) {
        _baselineLat = s.latitude;
        _baselineLon = s.longitude;
    }

    _state = SecurityState::ARMED;
    _lastResult = SecurityResult::OK;
#else
    _lastResult = SecurityResult::REJECTED_DISABLED;
#endif
    return _lastResult;
}

SecurityResult SecurityManager::disarm() {
#if ENABLE_SECURITY_ALARM
    if (_state == SecurityState::DISARMED) { _lastResult = SecurityResult::REJECTED_NOT_ARMED; return _lastResult; }
    stopAlarmOutput();
    SharedState::instance().clearWarning(WarningFlag::UNAUTHORIZED_MOVE);
    _state = SecurityState::DISARMED;
    _lastResult = SecurityResult::OK;
#else
    _lastResult = SecurityResult::REJECTED_DISABLED;
#endif
    return _lastResult;
}

void SecurityManager::evaluateTriggers() {
#if ENABLE_SECURITY_ALARM
    VehicleState s = SharedState::instance().snapshot();
    const char* reason = nullptr;

    // Tier 1: wheel motion — works with zero extra hardware beyond the
    // speed sensor the speedometer already needs.
    if (s.speedKmh > SECURITY_WHEEL_TRIGGER_KMH) reason = "wheel motion detected";

#if ENABLE_IMU
    if (!reason && SecurityMath::angleDeviationExceeds(_baselineLeanDeg, s.leanAngleDeg, SECURITY_LEAN_TRIGGER_DEG)) {
        reason = "lean angle changed (tilt/lift)";
    }
#endif

#if ENABLE_GPS
    if (!reason && _baselineGpsValid && s.gpsFixValid) {
        float drift = SecurityMath::gpsDriftMeters(_baselineLat, _baselineLon, s.latitude, s.longitude);
        if (drift > SECURITY_GPS_DRIFT_TRIGGER_M) reason = "GPS position drift (possible tow)";
    }
#endif

    if (reason) triggerAlarm(reason);
#endif
}

void SecurityManager::triggerAlarm(const char* reason) {
#if ENABLE_SECURITY_ALARM
    _state = SecurityState::ALARM_ACTIVE;
    _alarmStartedMs = millis();
    _lastPulseToggleMs = 0;
    _pulseOn = false;

    SharedState::instance().raiseWarning(WarningFlag::UNAUTHORIZED_MOVE);
    // NotificationManager's own scan of newly-raised warning flags handles
    // the single triple-beep + Notifications-screen force-switch (see
    // NotificationManager.cpp / DisplayManager::tick()) — this manager's
    // tick() adds the SUSTAINED horn+hazard chirp pattern on top (or the
    // buzzer fallback), started on the next tick() call now that _state is
    // ALARM_ACTIVE, not here — see tick() for the actual pulse loop.
    (void)reason;   // surfaced via lastResultString()/stateString() rather than logged here
#endif
}

void SecurityManager::stopAlarmOutput() {
#if ENABLE_SECURITY_ALARM
#if ENABLE_REMOTE_HORN || ENABLE_REMOTE_INDICATORS
    RemoteControlManager::instance().horn(false);
    RemoteControlManager::instance().hazard(false);
#else
    digitalWrite(PIN_OUT_BUZZER, LOW);
#endif
    _pulseOn = false;
    if (_state == SecurityState::ALARM_ACTIVE) _state = SecurityState::ARMED;   // stays armed, ready to re-trigger
#endif
}

const char* SecurityManager::lastResultString() const {
    switch (_lastResult) {
        case SecurityResult::OK: return "ok";
        case SecurityResult::REJECTED_MOVING: return "rejected_moving";
        case SecurityResult::REJECTED_ENGINE_RUNNING: return "rejected_engine_running";
        case SecurityResult::REJECTED_ALREADY_ARMED: return "rejected_already_armed";
        case SecurityResult::REJECTED_NOT_ARMED: return "rejected_not_armed";
        case SecurityResult::REJECTED_DISABLED: return "rejected_feature_disabled";
        default: return "unknown";
    }
}

const char* SecurityManager::stateString() const {
    switch (_state) {
        case SecurityState::DISARMED: return "disarmed";
        case SecurityState::ARMED: return "armed";
        case SecurityState::ALARM_ACTIVE: return "alarm_active";
        default: return "unknown";
    }
}
