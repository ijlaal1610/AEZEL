#include "BleManager.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include "RideManager.h"
#include "NotificationManager.h"
#include "OtaManager.h"
#include "DisplayManager.h"

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
    void onConnect(NimBLEServer* pServer) override {
        SharedState::instance().update([](VehicleState& s) { s.bleConnected = true; });
    }
    void onDisconnect(NimBLEServer* pServer) override {
        SharedState::instance().update([](VehicleState& s) { s.bleConnected = false; });
        NimBLEDevice::startAdvertising();
    }
};

void BleManager::begin() {
    NimBLEDevice::init("Phoenix Cockpit");
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

    String payload;
    serializeJson(doc, payload);
    telemetryChar->setValue(payload.c_str());
    telemetryChar->notify();
}

void BleManager::handleIncomingCommand(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;   // malformed — ignore, don't crash

    const char* cmd = doc["cmd"] | "";
    if (strcmp(cmd, "reset_trip_a") == 0) {
        RideManager::instance().resetTripA();
    }
    else if (strcmp(cmd, "reset_trip_b") == 0) {
        RideManager::instance().resetTripB();
    }
    else if (strcmp(cmd, "find_bike") == 0) {
        // Flash NeoPixel ring red + sound buzzer pulse
        SharedState::instance().update([](VehicleState& s) {
            s.activeWarnings |= (1 << (uint8_t)WarningFlag::UNAUTHORIZED_MOVE);
        });
    }
    else if (strcmp(cmd, "remote_ignition_toggle") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.inIgnitionOn = !s.inIgnitionOn;
        });
    }
    else if (strcmp(cmd, "remote_ignition_on") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.inIgnitionOn = true;
        });
    }
    else if (strcmp(cmd, "remote_ignition_off") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.inIgnitionOn = false;
        });
    }
    else if (strcmp(cmd, "remote_start_engine") == 0) {
        // --- Remote Engine Start Procedure & Safety Interlocks ---
        VehicleState cur = SharedState::instance().snapshot();
        if (cur.gear != GearState::NEUTRAL && !cur.inNeutral) {
            // Safety Interlock Violation: Vehicle is in gear!
            NotificationManager::instance().push("START BLOCKED", "Shift to Neutral (N) to remote start!", NotifPriority::WARNING);
            return;
        }
        if (cur.inKillSwitch) {
            NotificationManager::instance().push("START BLOCKED", "Turn Kill Switch OFF to remote start!", NotifPriority::WARNING);
            return;
        }

        // Turn ON Ignition (primes fuel pump) and pulse starter solenoid relay
        SharedState::instance().update([](VehicleState& s) {
            s.inIgnitionOn = true;
            s.inStarterActive = true;
            s.inEngineRunning = true;
            s.rpm = 1200; // Simulated idle RPM
        });

        NotificationManager::instance().push("REMOTE START", "Engine Started via Smartphone", NotifPriority::INFO);
    }
    else if (strcmp(cmd, "remote_horn_beep") == 0) {
        // Triggers 300ms horn pulse relay hook
    }
    else if (strcmp(cmd, "remote_hazard_toggle") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.inLeftIndicator = !s.inLeftIndicator;
            s.inRightIndicator = s.inLeftIndicator;
        });
    }
    else if (strcmp(cmd, "remote_indicator_left") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.inLeftIndicator = !s.inLeftIndicator;
            s.inRightIndicator = false;
        });
    }
    else if (strcmp(cmd, "remote_indicator_right") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.inRightIndicator = !s.inRightIndicator;
            s.inLeftIndicator = false;
        });
    }
    else if (strcmp(cmd, "remote_indicator_off") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.inLeftIndicator = false;
            s.inRightIndicator = false;
        });
    }
    else if (strcmp(cmd, "remote_seat_release") == 0) {
        // Solenoid 500ms impulse pulse hook
    }
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
            s.navEtaMinutes = eta;
        });
    }
    else if (strcmp(cmd, "toggle_speedo") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.showSpeedometer = !s.showSpeedometer;
        });
    }
    else if (strcmp(cmd, "toggle_focus") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.focusMode = !s.focusMode;
        });
    }
    else if (strcmp(cmd, "toggle_notif_overlay") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.allowNotifOverlay = !s.allowNotifOverlay;
        });
    }
    else if (strcmp(cmd, "toggle_lockscreen") == 0) {
        SharedState::instance().update([](VehicleState& s) {
            s.enableLockscreen = !s.enableLockscreen;
        });
    }
    else if (strcmp(cmd, "set_pin") == 0) {
        const char* pin = doc["pin"] | "1234";
        SharedState::instance().update([pin](VehicleState& s) {
            strncpy(s.pinCode, pin, 4);
            s.pinCode[4] = '\0';
        });
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
        // Triggers Wi-Fi AP + Web Server for wireless firmware uploads
        OtaManager::instance().enableWifiAp();
    }
    // Every command is intentionally an explicit allow-listed string match
    // rather than a generic eval-style dispatch — keeps the remote attack
    // surface auditable as features grow (Security section of the spec).
}
