#include "BleManager.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include "RideManager.h"
#include "NotificationManager.h"
#include "OtaManager.h"
#include "DisplayManager.h"
#include "RemoteControlManager.h"
#include "StorageManager.h"
#include "VehicleConfig.h"

#define SVC_UUID        "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_TELEMETRY  "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   // notify
#define CHAR_COMMAND    "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   // write

static NimBLECharacteristic* telemetryChar = nullptr;

class CommandCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
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
    NimBLEDevice::init("AEZEL Cockpit");
    NimBLEDevice::setMTU(256);
    NimBLEDevice::setSecurityAuth(true, true, true);

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    NimBLEService* svc = server->createService(SVC_UUID);
    telemetryChar = svc->createCharacteristic(CHAR_TELEMETRY, NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic* cmdChar = svc->createCharacteristic(
        CHAR_COMMAND, 
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
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

void BleManager::tick() { }

void BleManager::publishTelemetry() {
    if (!telemetryChar) return;
    VehicleState s = SharedState::instance().snapshot();
    if (!s.bleConnected) return;

    JsonDocument doc;
    doc["spd"]       = (int)s.speedKmh;
    doc["rpm"]       = s.rpm;
    doc["fuel"]      = (int)s.fuelLevelPct;
    doc["fuel_rng"]  = (int)s.fuelRangeKm;
    doc["batt"]      = s.batteryVoltage;
    doc["eng_t"]     = (int)s.engineTempC;
    doc["odo"]       = s.odometer_km;
    doc["tripA"]     = s.tripA_km;
    doc["tripB"]     = s.tripB_km;
    doc["max_spd"]   = (int)s.maxSpeedKmh;
    doc["avg_spd"]   = (int)s.avgSpeedKmh;
    doc["lean"]      = s.leanAngleDeg;
    doc["warn"]      = s.activeWarnings;
    doc["lat"]       = s.latitude;
    doc["lon"]       = s.longitude;
    doc["ign"]       = s.inIgnitionOn;
    doc["gear"]      = gearToString(s.gear);

    // Multi-vehicle platform fields
    doc["v_type"]    = (uint8_t)VehicleConfig::instance().type();
    doc["v_name"]    = VehicleConfig::instance().active().name;
    doc["d_src"]     = (uint8_t)VehicleConfig::instance().dataSource();

    // OBD-II fields
    if (VehicleConfig::instance().usesObd()) {
        doc["throttle"]  = (int)s.throttlePct;
        doc["load"]      = (int)s.engineLoadPct;
        doc["coolant"]   = (int)s.coolantTempC;
        doc["obd_ok"]    = s.obdConnected;
    }

    doc["show_spd"]  = s.showSpeedometer;
    doc["focus"]     = s.focusMode;
    doc["notif_ovl"] = s.allowNotifOverlay;
    doc["lock_en"]   = s.enableLockscreen;
    doc["locked"]    = s.isLocked;
    doc["cmd_res"]   = RemoteControlManager::instance().lastResultString();

    String payload;
    serializeJson(doc, payload);
    telemetryChar->setValue(payload.c_str());
    telemetryChar->notify();
}

void BleManager::handleIncomingCommand(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;

    const char* cmd = doc["cmd"] | "";

    // Trip operations
    if (strcmp(cmd, "reset_trip_a") == 0) {
        RideManager::instance().resetTripA();
    }
    else if (strcmp(cmd, "reset_trip_b") == 0) {
        RideManager::instance().resetTripB();
    }
    // Anti-theft "Find Bike" — raise warning & pulse hazard
    else if (strcmp(cmd, "find_bike") == 0) {
        SharedState::instance().raiseWarning(WarningFlag::UNAUTHORIZED_MOVE);
        RemoteControlManager::instance().hazard(true);
        NotificationManager::instance().push("FIND VEHICLE", "Hazard lights & warning active", NotifPriority::INFO);
    }
    // Remote Ignition Controls with speed interlock
    else if (strcmp(cmd, "remote_ignition_on") == 0) {
        SharedState::instance().update([](VehicleState& s) { s.inIgnitionOn = true; });
    }
    else if (strcmp(cmd, "remote_ignition_off") == 0) {
        VehicleState s = SharedState::instance().snapshot();
        if (s.speedKmh > 3.0f) {
            NotificationManager::instance().push("IGNITION CUT REFUSED", "Vehicle is in motion!", NotifPriority::CRITICAL);
            return;
        }
        SharedState::instance().update([](VehicleState& st) { st.inIgnitionOn = false; });
        RemoteControlManager::instance().remoteStop();
    }
    // Remote Engine Start via Centralized RemoteControlManager
    else if (strcmp(cmd, "remote_start_engine") == 0) {
        RemoteResult res = RemoteControlManager::instance().remoteStart();
        if (res == RemoteResult::OK) {
            NotificationManager::instance().push("REMOTE START", "Engine Started via Smartphone", NotifPriority::INFO);
        } else {
            NotificationManager::instance().push("START REFUSED", RemoteControlManager::instance().lastResultString(), NotifPriority::WARNING);
        }
    }
    else if (strcmp(cmd, "remote_stop_engine") == 0) {
        RemoteControlManager::instance().remoteStop();
    }
    // Remote Actuators via Centralized RemoteControlManager
    else if (strcmp(cmd, "remote_horn_beep") == 0) {
        RemoteControlManager::instance().horn(true);
        vTaskDelay(pdMS_TO_TICKS(300));
        RemoteControlManager::instance().horn(false);
    }
    else if (strcmp(cmd, "remote_hazard_toggle") == 0) {
        static bool s_haz = false;
        s_haz = !s_haz;
        RemoteControlManager::instance().hazard(s_haz);
    }
    else if (strcmp(cmd, "remote_indicator_left") == 0) {
        RemoteControlManager::instance().indicator(true, true);
    }
    else if (strcmp(cmd, "remote_indicator_right") == 0) {
        RemoteControlManager::instance().indicator(false, true);
    }
    else if (strcmp(cmd, "remote_indicator_off") == 0) {
        RemoteControlManager::instance().indicator(true, false);
        RemoteControlManager::instance().indicator(false, false);
    }
    // Multi-Vehicle Profile Switching
    else if (strcmp(cmd, "set_vehicle_profile") == 0) {
        if (doc.containsKey("preset")) {
            size_t idx = doc["preset"] | 0;
            if (VehicleConfig::instance().setProfileByIndex(idx)) {
                NotificationManager::instance().push("PROFILE SWITCHED", VehicleConfig::instance().active().name, NotifPriority::INFO);
            }
        }
    }
    // Navigation Turn-by-Turn Relay
    else if (strcmp(cmd, "nav_update") == 0) {
        uint32_t dist = doc["dist"] | 0;
        uint8_t turn = doc["turn"] | 0;
        const char* street = doc["street"] | "";
        uint16_t eta = doc["eta"] | 0;

        SharedState::instance().update([=](VehicleState& s) {
            s.navActive = true;
            s.navDistanceMeters = dist;
            s.navTurnIcon = turn;
            strncpy(s.navStreetName, street, sizeof(s.navStreetName) - 1);
            s.navStreetName[sizeof(s.navStreetName) - 1] = '\0';
            s.navEtaMinutes = eta;
        });
    }
    // Customization Toggles
    else if (strcmp(cmd, "toggle_speedo") == 0) {
        SharedState::instance().update([](VehicleState& s) { s.showSpeedometer = !s.showSpeedometer; });
        StorageManager::instance().saveUserSettings();
    }
    else if (strcmp(cmd, "toggle_focus") == 0) {
        SharedState::instance().update([](VehicleState& s) { s.focusMode = !s.focusMode; });
        StorageManager::instance().saveUserSettings();
    }
    else if (strcmp(cmd, "toggle_notif_overlay") == 0) {
        SharedState::instance().update([](VehicleState& s) { s.allowNotifOverlay = !s.allowNotifOverlay; });
        StorageManager::instance().saveUserSettings();
    }
    else if (strcmp(cmd, "toggle_lockscreen") == 0) {
        SharedState::instance().update([](VehicleState& s) { s.enableLockscreen = !s.enableLockscreen; });
        StorageManager::instance().saveUserSettings();
    }
    else if (strcmp(cmd, "set_pin") == 0) {
        const char* pin = doc["pin"] | "";
        if (strlen(pin) == 4) {
            SharedState::instance().update([pin](VehicleState& s) {
                strncpy(s.pinCode, pin, 4);
                s.pinCode[4] = '\0';
            });
            StorageManager::instance().saveUserSettings();
            NotificationManager::instance().push("SECURITY PIN", "PIN successfully updated", NotifPriority::INFO);
        }
    }
    else if (strcmp(cmd, "open_drawer") == 0) {
        DisplayManager::instance().toggleMenuDrawer();
    }
    else if (strcmp(cmd, "phone_notif") == 0) {
        const char* appName = doc["app"] | "Phone";
        const char* title = doc["title"] | "";
        const char* body = doc["body"] | "";
        String notifTitle = String(appName) + ": " + String(title);
        NotificationManager::instance().push(notifTitle, String(body), NotifPriority::INFO);
    }
    else if (strcmp(cmd, "remote_ota_wifi") == 0) {
        VehicleState s = SharedState::instance().snapshot();
        if (s.speedKmh > 1.0f || s.inEngineRunning) {
            NotificationManager::instance().push("OTA REFUSED", "Cannot flash while engine is running", NotifPriority::WARNING);
            return;
        }
        OtaManager::instance().enableWifiAp();
    }
}
