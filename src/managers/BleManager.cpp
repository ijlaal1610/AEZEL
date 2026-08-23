#include "BleManager.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include "RideManager.h"
#include "RemoteControlManager.h"
#include "MaintenanceManager.h"
#include "SecurityManager.h"
#include "PhoneLinkManager.h"
#include "NotificationManager.h"
#include "DisplayPolicyMath.h"

// Custom 128-bit UUIDs — regenerate for your own build to avoid clashing
// with anyone reusing this firmware on the same channel during development.
#define SVC_UUID        "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_TELEMETRY  "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   // notify
#define CHAR_COMMAND    "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   // write

static NimBLECharacteristic* telemetryChar = nullptr;

class CommandCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        BleManager::instance().tick();   // no-op hook; real dispatch below
        std::string v = c->getValue();
        BleManager::instance().handleIncomingCommand(String(v.c_str()));
    }
};

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer*, NimBLEConnInfo&) override {
        SharedState::instance().update([](VehicleState& s) { s.bleConnected = true; });
    }
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
        SharedState::instance().update([](VehicleState& s) { s.bleConnected = false; });
        NimBLEDevice::startAdvertising();
    }
};

void BleManager::begin() {
    NimBLEDevice::init("AEZEL");
    NimBLEDevice::setSecurityAuth(true, true, true);   // bonding + MITM protection for PIN unlock feature

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    NimBLEService* svc = server->createService(SVC_UUID);
    telemetryChar = svc->createCharacteristic(CHAR_TELEMETRY, NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic* cmdChar = svc->createCharacteristic(CHAR_COMMAND, NIMBLE_PROPERTY::WRITE);
    cmdChar->setCallbacks(new CommandCallbacks());

    svc->start();
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(SVC_UUID);
    adv->start();

    _started = true;
}

void BleManager::taskEntry(void* pv) {
    BleManager& self = instance();
    if (!self._started) self.begin();
    const TickType_t period = pdMS_TO_TICKS(500);   // 2 Hz telemetry — plenty for a companion app
    for (;;) {
        self.publishTelemetry();
        vTaskDelay(period);
    }
}

void BleManager::tick() { /* reserved for future periodic BLE housekeeping */ }

void BleManager::publishTelemetry() {
    if (!telemetryChar) return;
    VehicleState s = SharedState::instance().snapshot();
    if (!s.bleConnected) return;

    StaticJsonDocument<768> doc;
    doc["spd"] = (int)s.speedKmh;
    doc["rpm"] = s.rpm;
    doc["fuel"] = (int)s.fuelLevelPct;
    doc["batt"] = s.batteryVoltage;
    doc["eng_t"] = s.engineTempC;
    doc["odo"] = s.odometer_km;
    doc["warn"] = s.activeWarnings;
    doc["lat"] = s.latitude;
    doc["lon"] = s.longitude;
    doc["cmd_result"] = RemoteControlManager::instance().lastResultString();
    doc["svc_due_km"] = MaintenanceManager::instance().serviceDueKm();
    doc["chain_due_km"] = MaintenanceManager::instance().chainDueKm();
    doc["security_state"] = SecurityManager::instance().stateString();

    // Phone-link outbound action (accept/reject a call, play/pause/skip
    // music) — consumed here so it's delivered exactly once on the next
    // telemetry tick after the dash button press, not resent every 500ms.
    String action = PhoneLinkManager::instance().consumePendingAction();
    if (action.length() > 0) doc["phone_action"] = action;

    // Mirror phone-link state back — same freshness rule DisplayManager
    // uses for its widgets (see DisplayManager::refreshWidgetsFromState()),
    // so the app's idea of "is the nav banner currently showing" matches
    // what's actually on the dash rather than drifting after a reconnect.
    PhoneLinkState link = PhoneLinkManager::instance().snapshot();
    uint32_t now = millis();
    doc["call_active"] = link.callActive && DisplayPolicyMath::isDataFresh(link.callUpdatedMs, now, CALL_STALE_MS);
    if (link.callActive) doc["caller_name"] = link.callerName;
    if (DisplayPolicyMath::isDataFresh(link.musicUpdatedMs, now, PHONE_LINK_STALE_MS)) {
        doc["music_title"] = link.musicTitle;
        doc["music_artist"] = link.musicArtist;
        doc["music_playing"] = link.musicPlaying;
    }
    if (DisplayPolicyMath::isDataFresh(link.navUpdatedMs, now, PHONE_LINK_STALE_MS)) {
        doc["nav_instruction"] = link.navInstruction;
        doc["nav_distance_m"] = link.navDistanceM;
    }

    String payload;
    serializeJson(doc, payload);
    telemetryChar->setValue(payload.c_str());
    telemetryChar->notify();
}

void BleManager::handleIncomingCommand(const String& json) {
    StaticJsonDocument<320> doc;
    if (deserializeJson(doc, json)) return;   // malformed — ignore, don't crash

    const char* cmd = doc["cmd"] | "";
    auto& remote = RemoteControlManager::instance();
    auto& maint = MaintenanceManager::instance();
    auto& phone = PhoneLinkManager::instance();

    if (strcmp(cmd, "reset_trip_a") == 0) RideManager::instance().resetTripA();
    else if (strcmp(cmd, "reset_trip_b") == 0) RideManager::instance().resetTripB();
    else if (strcmp(cmd, "find_bike") == 0) { remote.horn(true); remote.hazard(true); }
    else if (strcmp(cmd, "horn_on") == 0) remote.horn(true);
    else if (strcmp(cmd, "horn_off") == 0) remote.horn(false);
    else if (strcmp(cmd, "hazard_on") == 0) remote.hazard(true);
    else if (strcmp(cmd, "hazard_off") == 0) remote.hazard(false);
    else if (strcmp(cmd, "indicator_left_on") == 0) remote.indicator(true, true);
    else if (strcmp(cmd, "indicator_left_off") == 0) remote.indicator(true, false);
    else if (strcmp(cmd, "indicator_right_on") == 0) remote.indicator(false, true);
    else if (strcmp(cmd, "indicator_right_off") == 0) remote.indicator(false, false);
    else if (strcmp(cmd, "lock") == 0) remote.setLocked(true);
    else if (strcmp(cmd, "unlock") == 0) remote.setLocked(false);
    else if (strcmp(cmd, "remote_start") == 0) remote.remoteStart();
    else if (strcmp(cmd, "remote_stop") == 0) remote.remoteStop();
    else if (strcmp(cmd, "set_tyre_due_km") == 0) maint.setTyreDueKm(doc["value"] | 0u);
    else if (strcmp(cmd, "set_insurance_due_ts") == 0) maint.setInsuranceDueTs(doc["value"] | 0u);
    else if (strcmp(cmd, "set_puc_due_ts") == 0) maint.setPucDueTs(doc["value"] | 0u);
    else if (strcmp(cmd, "mark_serviced") == 0) maint.markServiced();
    else if (strcmp(cmd, "mark_chain_lubed") == 0) maint.markChainLubed();
    else if (strcmp(cmd, "arm_security") == 0) SecurityManager::instance().arm();
    else if (strcmp(cmd, "disarm_security") == 0) SecurityManager::instance().disarm();
    else if (strcmp(cmd, "incoming_call") == 0) phone.showIncomingCall(String(doc["caller"] | "Unknown"));
    else if (strcmp(cmd, "call_ended") == 0) phone.endCall();
    else if (strcmp(cmd, "push_message") == 0) {
        // Routed into the existing NotificationManager queue rather than a
        // separate message list — it's already exactly "a title, a body,
        // and a priority, shown in the Notifications screen list, tap to
        // dismiss," which is precisely what a message preview needs. No
        // new UI required for this one.
        const char* app = doc["app"] | "Message";
        const char* sender = doc["sender"] | "";
        const char* preview = doc["preview"] | "";
        char title[48];
        snprintf(title, sizeof(title), "%s: %s", app, sender);
        NotificationManager::instance().push(title, preview, NotifPriority::INFO);
    }
    else if (strcmp(cmd, "music_update") == 0) {
        phone.updateMusic(String(doc["title"] | ""),
                           String(doc["artist"] | ""),
                           doc["playing"] | false);
    }
    else if (strcmp(cmd, "nav_update") == 0) {
        phone.updateNavigation(String(doc["instruction"] | ""), doc["distance_m"] | 0.0f);
    }
    else if (strcmp(cmd, "nav_end") == 0) phone.endNavigation();
    // Every command is intentionally an explicit allow-listed string match
    // rather than a generic eval-style dispatch — keeps the remote attack
    // surface auditable as features grow (Security section of the spec).
    // Every actuator command routes through RemoteControlManager, never a
    // raw GPIO write here — that's where the safety interlocks live (see
    // docs/remote_control.md). Maintenance commands are pure data entry
    // (no actuator, no interlock needed) so they route directly to
    // MaintenanceManager instead, and phone-link commands (call/music/nav)
    // are pure data mirroring, routed to PhoneLinkManager — see
    // docs/phone_link.md for the full command contract. The result
    // (including WHY an actuator command was refused) rides back to the
    // app in the next telemetry packet's "cmd_result" field rather than
    // being silently dropped.
}
