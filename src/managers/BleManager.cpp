#include "BleManager.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include "RideManager.h"
#include "RemoteControlManager.h"

// Custom 128-bit UUIDs — kept in exact sync with android-app & gemini-android-app
#define SVC_UUID        "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_TELEMETRY  "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   // notify
#define CHAR_COMMAND    "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   // write

static NimBLECharacteristic* telemetryChar = nullptr;

class CommandCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        BleManager::instance().tick();
        std::string v = c->getValue();
        BleManager::instance().handleIncomingCommand(String(v.c_str()));
    }
};

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        SharedState::instance().update([](VehicleState& s) { s.bleConnected = true; });
    }
    void onDisconnect(NimBLEServer* pServer) override {
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
    const TickType_t period = pdMS_TO_TICKS(500);   // 2 Hz telemetry
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

    JsonDocument doc;
    doc["spd"] = (int)s.speedKmh;
    doc["rpm"] = s.rpm;
    doc["fuel"] = (int)s.fuelLevelPct;
    doc["fuel_rng"] = (int)s.fuelRangeKm;
    doc["batt"] = s.batteryVoltage;
    doc["eng_t"] = (int)s.engineTempC;
    doc["odo"] = s.odometer_km;
    doc["tripA"] = s.tripA_km;
    doc["tripB"] = s.tripB_km;
    doc["max_spd"] = (int)s.maxSpeedKmh;
    doc["avg_spd"] = (int)s.avgSpeedKmh;
    doc["lean"] = s.leanAngleDeg;
    doc["warn"] = s.activeWarnings;
    doc["lat"] = s.latitude;
    doc["lon"] = s.longitude;
    doc["ign"] = s.inIgnitionOn;

    const char* gearStr = "N";
    switch (s.gear) {
        case GearState::GEAR_1: gearStr = "1"; break;
        case GearState::GEAR_2: gearStr = "2"; break;
        case GearState::GEAR_3: gearStr = "3"; break;
        case GearState::GEAR_4: gearStr = "4"; break;
        case GearState::GEAR_5: gearStr = "5"; break;
        case GearState::NEUTRAL: gearStr = "N"; break;
        default: gearStr = "-"; break;
    }
    doc["gear"] = gearStr;
    doc["show_spd"] = s.showSpeedometer;
    doc["focus"] = s.focusMode;
    doc["notif_ovl"] = s.allowNotifOverlay;
    doc["lock_en"] = s.enableLockscreen;
    doc["locked"] = s.isLocked;
    doc["cmd_result"] = RemoteControlManager::instance().lastResultString();

    String payload;
    serializeJson(doc, payload);
    telemetryChar->setValue(payload.c_str());
    telemetryChar->notify();
}

void BleManager::handleIncomingCommand(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;

    const char* cmd = doc["cmd"] | "";
    auto& remote = RemoteControlManager::instance();

    if (strcmp(cmd, "reset_trip_a") == 0) RideManager::instance().resetTripA();
    else if (strcmp(cmd, "reset_trip_b") == 0) RideManager::instance().resetTripB();
    else if (strcmp(cmd, "find_bike") == 0) { remote.horn(true); remote.hazard(true); }
    else if (strcmp(cmd, "horn_on") == 0 || strcmp(cmd, "remote_horn_beep") == 0) remote.horn(true);
    else if (strcmp(cmd, "horn_off") == 0) remote.horn(false);
    else if (strcmp(cmd, "hazard_on") == 0 || strcmp(cmd, "remote_hazard_toggle") == 0) remote.hazard(true);
    else if (strcmp(cmd, "hazard_off") == 0) remote.hazard(false);
    else if (strcmp(cmd, "indicator_left_on") == 0) remote.indicator(true, true);
    else if (strcmp(cmd, "indicator_left_off") == 0) remote.indicator(true, false);
    else if (strcmp(cmd, "indicator_right_on") == 0) remote.indicator(false, true);
    else if (strcmp(cmd, "indicator_right_off") == 0) remote.indicator(false, false);
    else if (strcmp(cmd, "lock") == 0 || strcmp(cmd, "toggle_lockscreen") == 0) remote.setLocked(true);
    else if (strcmp(cmd, "unlock") == 0) remote.setLocked(false);
    else if (strcmp(cmd, "remote_start") == 0 || strcmp(cmd, "remote_start_engine") == 0) remote.remoteStart();
    else if (strcmp(cmd, "remote_stop") == 0) remote.remoteStop();
    else if (strcmp(cmd, "remote_ignition_toggle") == 0) {
        SharedState::instance().update([](VehicleState& s) { s.inIgnitionOn = !s.inIgnitionOn; });
    }
    else if (strcmp(cmd, "remote_seat_release") == 0) {
        remote.hazard(true);
    }
    else if (strcmp(cmd, "toggle_speedo") == 0) {
        SharedState::instance().update([](VehicleState& s) { s.showSpeedometer = !s.showSpeedometer; });
    }
    else if (strcmp(cmd, "toggle_focus") == 0) {
        SharedState::instance().update([](VehicleState& s) { s.focusMode = !s.focusMode; });
    }
    else if (strcmp(cmd, "toggle_notif_overlay") == 0) {
        SharedState::instance().update([](VehicleState& s) { s.allowNotifOverlay = !s.allowNotifOverlay; });
    }
    else if (strcmp(cmd, "set_pin") == 0) {
        const char* pin = doc["pin"] | "1234";
        SharedState::instance().update([&](VehicleState& s) {
            strncpy(s.pinCode, pin, 4);
            s.pinCode[4] = '\0';
        });
    }
}
