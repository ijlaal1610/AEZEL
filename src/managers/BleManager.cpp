#include "BleManager.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include "RideManager.h"
#include "RemoteControlManager.h"

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

    StaticJsonDocument<384> doc;
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

    String payload;
    serializeJson(doc, payload);
    telemetryChar->setValue(payload.c_str());
    telemetryChar->notify();
}

void BleManager::handleIncomingCommand(const String& json) {
    StaticJsonDocument<192> doc;
    if (deserializeJson(doc, json)) return;   // malformed — ignore, don't crash

    const char* cmd = doc["cmd"] | "";
    auto& remote = RemoteControlManager::instance();

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
    // Every command is intentionally an explicit allow-listed string match
    // rather than a generic eval-style dispatch — keeps the remote attack
    // surface auditable as features grow (Security section of the spec).
    // Every actuator command routes through RemoteControlManager, never a
    // raw GPIO write here — that's where the safety interlocks live (see
    // docs/remote_control.md). The result (including WHY something was
    // refused) rides back to the app in the next telemetry packet's
    // "cmd_result" field rather than being silently dropped.
}
