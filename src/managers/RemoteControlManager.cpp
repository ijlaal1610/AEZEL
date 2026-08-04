#include "RemoteControlManager.h"
#include "Config.h"

void RemoteControlManager::begin() {
#if ENABLE_REMOTE_HORN
    pinMode(PIN_OUT_HORN_RELAY, OUTPUT);
    digitalWrite(PIN_OUT_HORN_RELAY, LOW);
#endif
#if ENABLE_REMOTE_INDICATORS
    pinMode(PIN_OUT_LEFT_IND_RELAY, OUTPUT);
    pinMode(PIN_OUT_RIGHT_IND_RELAY, OUTPUT);
    digitalWrite(PIN_OUT_LEFT_IND_RELAY, LOW);
    digitalWrite(PIN_OUT_RIGHT_IND_RELAY, LOW);
#endif
#if ENABLE_REMOTE_IMMOBILIZER
    pinMode(PIN_OUT_IMMOBILIZER_RELAY, OUTPUT);
    // Relay is normally-closed in the ignition-enable circuit (see
    // docs/remote_control.md) — driving the pin LOW here means "unlocked,"
    // matching the fail-safe requirement: a dead ESP32/brownout leaves the
    // rider able to start the bike, never stranded.
    digitalWrite(PIN_OUT_IMMOBILIZER_RELAY, LOW);
#endif
#if ENABLE_REMOTE_STARTER
    pinMode(PIN_OUT_STARTER_RELAY, OUTPUT);
    digitalWrite(PIN_OUT_STARTER_RELAY, LOW);
#endif
}

void RemoteControlManager::taskEntry(void* pv) {
    RemoteControlManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(200);   // 5 Hz — fast enough to react if an interlock breaks mid-start
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

bool RemoteControlManager::stationaryInterlockOk() const {
    VehicleState s = SharedState::instance().snapshot();
    return s.speedKmh < 1.0f;   // treat <1km/h as "not moving" (sensor noise floor)
}

void RemoteControlManager::tick() {
#if ENABLE_REMOTE_STARTER
    if (!_remoteStartActive) return;
    VehicleState s = SharedState::instance().snapshot();

    // Any of these breaking mid-start cuts the starter/ignition immediately —
    // a remote-started bike is not something that should keep running once
    // someone puts it in gear, raises the stand, or it starts moving.
    if (!s.inNeutral)              { cutRemoteStart("gear engaged"); return; }
    if (!s.inSideStand)             { cutRemoteStart("side stand raised"); return; }
    if (s.speedKmh >= 1.0f)          { cutRemoteStart("motion detected"); return; }
    if (s.inKillSwitch)               { cutRemoteStart("kill switch"); return; }
    if (millis() - _remoteStartBeganMs > MAX_UNATTENDED_RUN_MS) {
        cutRemoteStart("max unattended runtime reached");
        return;
    }
#endif
}

RemoteResult RemoteControlManager::horn(bool on) {
#if ENABLE_REMOTE_HORN
    digitalWrite(PIN_OUT_HORN_RELAY, on ? HIGH : LOW);
    _lastResult = RemoteResult::OK;
#else
    _lastResult = RemoteResult::REJECTED_DISABLED;
#endif
    return _lastResult;
}

RemoteResult RemoteControlManager::hazard(bool on) {
#if ENABLE_REMOTE_INDICATORS
    digitalWrite(PIN_OUT_LEFT_IND_RELAY, on ? HIGH : LOW);
    digitalWrite(PIN_OUT_RIGHT_IND_RELAY, on ? HIGH : LOW);
    _lastResult = RemoteResult::OK;
#else
    _lastResult = RemoteResult::REJECTED_DISABLED;
#endif
    return _lastResult;
}

RemoteResult RemoteControlManager::indicator(bool left, bool on) {
#if ENABLE_REMOTE_INDICATORS
    // Riding-indicator control belongs to the physical switch, always. Remote
    // indicator control is scoped to "find my bike in a parking lot" —
    // refused outright once the bike is moving, so a phone app can never
    // become a second, conflicting source of truth for turn signals while
    // riding.
    if (!stationaryInterlockOk()) { _lastResult = RemoteResult::REJECTED_MOVING; return _lastResult; }
    digitalWrite(left ? PIN_OUT_LEFT_IND_RELAY : PIN_OUT_RIGHT_IND_RELAY, on ? HIGH : LOW);
    _lastResult = RemoteResult::OK;
#else
    _lastResult = RemoteResult::REJECTED_DISABLED;
#endif
    return _lastResult;
}

RemoteResult RemoteControlManager::setLocked(bool lock) {
#if ENABLE_REMOTE_IMMOBILIZER
    if (lock) {
        // Locking (cutting ignition-enable) is only ever allowed at a
        // complete stop with the engine already off. Engaging this while
        // riding or with the engine running is exactly the failure mode
        // that makes remote immobilizers dangerous — it is refused
        // unconditionally, not just "discouraged."
        VehicleState s = SharedState::instance().snapshot();
        if (s.inEngineRunning) { _lastResult = RemoteResult::REJECTED_ENGINE_RUNNING; return _lastResult; }
        if (!stationaryInterlockOk()) { _lastResult = RemoteResult::REJECTED_MOVING; return _lastResult; }
        digitalWrite(PIN_OUT_IMMOBILIZER_RELAY, HIGH);
    } else {
        digitalWrite(PIN_OUT_IMMOBILIZER_RELAY, LOW);   // unlock is always allowed
    }
    _lastResult = RemoteResult::OK;
#else
    _lastResult = RemoteResult::REJECTED_DISABLED;
#endif
    return _lastResult;
}

RemoteResult RemoteControlManager::remoteStart() {
#if ENABLE_REMOTE_STARTER
    if (_remoteStartActive) { _lastResult = RemoteResult::REJECTED_ALREADY_ACTIVE; return _lastResult; }

    VehicleState s = SharedState::instance().snapshot();
    if (s.inEngineRunning)      { _lastResult = RemoteResult::REJECTED_ENGINE_RUNNING; return _lastResult; }
    if (!s.inNeutral)            { _lastResult = RemoteResult::REJECTED_NOT_NEUTRAL; return _lastResult; }
    if (!s.inSideStand)           { _lastResult = RemoteResult::REJECTED_SIDESTAND_DOWN; return _lastResult; }
    if (s.inKillSwitch)            { _lastResult = RemoteResult::REJECTED_KILL_SWITCH; return _lastResult; }

    // All interlocks satisfied: neutral confirmed, side stand down (bike
    // physically can't move even if bumped into gear), kill switch not
    // engaged, not already running. Pulse the starter relay briefly — a
    // real implementation should pulse for ~1-2s like a normal starter
    // press, not hold it, and should watch RPM to know when to release.
    digitalWrite(PIN_OUT_STARTER_RELAY, HIGH);
    delay(1500);
    digitalWrite(PIN_OUT_STARTER_RELAY, LOW);

    _remoteStartActive = true;
    _remoteStartBeganMs = millis();
    _lastResult = RemoteResult::OK;
#else
    _lastResult = RemoteResult::REJECTED_DISABLED;
#endif
    return _lastResult;
}

RemoteResult RemoteControlManager::remoteStop() {
#if ENABLE_REMOTE_STARTER
    cutRemoteStart("app requested stop");
    _lastResult = RemoteResult::OK;
#else
    _lastResult = RemoteResult::REJECTED_DISABLED;
#endif
    return _lastResult;
}

void RemoteControlManager::cutRemoteStart(const char* reason) {
#if ENABLE_REMOTE_STARTER
    digitalWrite(PIN_OUT_STARTER_RELAY, LOW);
    _remoteStartActive = false;
    // A real build should also drive ignition-enable low here via the same
    // immobilizer relay used by setLocked() — cutting spark, not just
    // releasing the starter — and push `reason` to NotificationManager so
    // the rider sees why it stopped next time they look at the dash.
    (void)reason;
#endif
}

const char* RemoteControlManager::lastResultString() const {
    switch (_lastResult) {
        case RemoteResult::OK: return "ok";
        case RemoteResult::REJECTED_MOVING: return "rejected_moving";
        case RemoteResult::REJECTED_ENGINE_RUNNING: return "rejected_engine_running";
        case RemoteResult::REJECTED_NOT_NEUTRAL: return "rejected_not_neutral";
        case RemoteResult::REJECTED_SIDESTAND_DOWN: return "rejected_sidestand_down";
        case RemoteResult::REJECTED_ALREADY_ACTIVE: return "rejected_already_active";
        case RemoteResult::REJECTED_DISABLED: return "rejected_feature_disabled";
        case RemoteResult::REJECTED_KILL_SWITCH: return "rejected_kill_switch";
        default: return "unknown";
    }
}
