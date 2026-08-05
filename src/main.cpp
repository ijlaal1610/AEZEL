// ============================================================================
//  AEZEL — ESP32 Smart Motorcycle Cockpit
//  main.cpp — boot sequence + FreeRTOS task graph
//
//  Boot budget target: < 1.5s from power-on to a visible, responsive
//  dashboard (fast-boot requirement). Heavy/non-critical managers (GPS lock,
//  BLE advertising, SD mount) initialize in parallel tasks rather than
//  blocking the main thread — the display comes up first, showing sensible
//  "acquiring..." placeholders that fill in as each subsystem comes online.
// ============================================================================
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "Config.h"
#include "DataModel.h"

#include "managers/SensorManager.h"
#include "managers/RideManager.h"
#include "managers/PowerManager.h"
#include "managers/StorageManager.h"
#include "managers/LightingManager.h"
#include "managers/NotificationManager.h"
#include "managers/GpsManager.h"
#include "managers/BleManager.h"
#include "managers/DisplayManager.h"
#if ENABLE_OTA_WIFI
#include "managers/OtaManager.h"
#endif
#if ENABLE_CAN_BUS
#include "managers/CanManager.h"
#endif

// Task handles kept for diagnostics (stack high-water-mark reporting, etc.)
static TaskHandle_t hDisplay, hSensor, hRide, hPower, hStorage, hLighting, hNotif, hGps, hBle, hDiag;

static void diagnosticsTask(void* pv) {
    for (;;) {
        SharedState::instance().update([](VehicleState& s) {
            s.freeHeapBytes = ESP.getFreeHeap();
        });
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== AEZEL — Smart Motorcycle Cockpit ===");

    esp_task_wdt_init(8, true);   // 8s watchdog, panic (reboot) on starvation
    esp_task_wdt_add(NULL);

    // --- Phase 1: fast-boot critical path -----------------------------
    StorageManager::instance().begin();     // mount NVS/SD before anyone reads settings
    PowerManager::instance().begin();
    SensorManager::instance().begin();
    RideManager::instance().begin();        // loads odometer/trip from NVS
    LightingManager::instance().begin();
    NotificationManager::instance().begin();
    DisplayManager::instance().begin();     // dashboard visible from here on

    // --- Phase 2: background subsystems (can lock/connect asynchronously) --
    GpsManager::instance().begin();
    // BleManager::begin() is called lazily inside its own task on first run

    LightingManager::instance().playWelcomeAnimation();

    // --- FreeRTOS task graph ---------------------------------------------
    // Real-time core (1): display + sensors + power + ride integration +
    // lighting + notifications — nothing here should ever block on network
    // or SD I/O for more than a few ms.
    xTaskCreatePinnedToCore(SensorManager::taskEntry,       "Sensor",   4096, nullptr, PRIO_SENSOR,      &hSensor,   CORE_REALTIME);
    xTaskCreatePinnedToCore(RideManager::taskEntry,         "Ride",     4096, nullptr, PRIO_SENSOR,      &hRide,     CORE_REALTIME);
    xTaskCreatePinnedToCore(PowerManager::taskEntry,        "Power",    3072, nullptr, PRIO_POWER,       &hPower,    CORE_REALTIME);
    xTaskCreatePinnedToCore(LightingManager::taskEntry,     "Lighting", 3072, nullptr, PRIO_SENSOR,      &hLighting, CORE_REALTIME);
    xTaskCreatePinnedToCore(NotificationManager::taskEntry, "Notif",    3072, nullptr, PRIO_SENSOR,      &hNotif,    CORE_REALTIME);
    xTaskCreatePinnedToCore(DisplayManager::taskEntry,      "Display",  8192, nullptr, PRIO_DISPLAY,     &hDisplay,  CORE_REALTIME);

    // Connectivity core (0): GPS parsing, BLE, SD-heavy storage flush,
    // diagnostics — anything that can stall on I/O lives here so it never
    // steals cycles from the render loop.
    xTaskCreatePinnedToCore(StorageManager::taskEntry, "Storage", 4096, nullptr, PRIO_STORAGE,     &hStorage, CORE_CONNECTIVITY);
    xTaskCreatePinnedToCore(GpsManager::taskEntry,      "GPS",     4096, nullptr, PRIO_GPS,          &hGps,     CORE_CONNECTIVITY);
    xTaskCreatePinnedToCore(BleManager::taskEntry,      "BLE",     6144, nullptr, PRIO_BLE,          &hBle,     CORE_CONNECTIVITY);
    xTaskCreatePinnedToCore(diagnosticsTask,             "Diag",    2048, nullptr, PRIO_DIAGNOSTICS,  &hDiag,    CORE_CONNECTIVITY);

#if ENABLE_CAN_BUS
    static TaskHandle_t hCan;
    xTaskCreatePinnedToCore(CanManager::taskEntry,      "CAN",     4096, nullptr, PRIO_SENSOR,       &hCan,     CORE_CONNECTIVITY);
#endif
#if ENABLE_OTA_WIFI
    static TaskHandle_t hOta;
    xTaskCreatePinnedToCore(OtaManager::taskEntry,      "OTA",     4096, nullptr, PRIO_STORAGE,      &hOta,     CORE_CONNECTIVITY);
#endif

    Serial.println("All tasks started. Boot complete.");
}

void loop() {
    // Intentionally empty — Arduino's loop() task is left idle at lowest
    // priority; all real work happens in the pinned FreeRTOS tasks above.
    // Feeding the watchdog here catches the (unlikely) case where every
    // other task has somehow starved the scheduler.
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1000));
}
