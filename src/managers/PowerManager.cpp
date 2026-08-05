#include "PowerManager.h"
#include "Config.h"
#include "StorageManager.h"
#include <esp_sleep.h>

void PowerManager::begin() {
    pinMode(PIN_IGNITION_SENSE, INPUT);
    _lastIgnitionState = digitalRead(PIN_IGNITION_SENSE);
    _state = PowerState::ACTIVE;
}

void PowerManager::taskEntry(void* pv) {
    PowerManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(200);   // 5 Hz is plenty for power state
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void PowerManager::tick() {
    bool ignitionOn = digitalRead(PIN_IGNITION_SENSE);
    if (ignitionOn != _lastIgnitionState) {
        handleIgnitionEdge(ignitionOn);
        _lastIgnitionState = ignitionOn;
    }

    SharedState::instance().update([&](VehicleState& s) { s.inIgnitionOn = ignitionOn; });

    switch (_state) {
        case PowerState::IGNITION_OFF_IDLE:
            if (millis() - _ignitionOffSinceMs > IDLE_LINGER_MS) {
                requestSafeShutdown();
            }
            break;
        default: break;
    }
}

void PowerManager::handleIgnitionEdge(bool ignitionOn) {
    if (ignitionOn) {
        // Ignition just turned ON — wake path (either cold boot already
        // happened via EXT0, or we were lingering in idle).
        _state = PowerState::ACTIVE;
    } else {
        // Ignition just turned OFF — start the linger timer instead of
        // sleeping immediately, so trip summary / goodbye animation / BLE
        // flush can complete cleanly.
        _state = PowerState::IGNITION_OFF_IDLE;
        _ignitionOffSinceMs = millis();
    }
}

#include "DisplayManager.h"

void PowerManager::requestSafeShutdown() {
    _state = PowerState::SHUTTING_DOWN;

    // 1) Flush ride data / settings so nothing is lost mid-write.
    StorageManager::instance().flushAll();

    // 2) Display Post-Ride Summary card & Goodbye animation on TFT display
    DisplayManager::instance().showRideSummaryScreen();
    DisplayManager::instance().showGoodbyeScreen();

    enterDeepSleep();
}

void PowerManager::enterDeepSleep() {
    // Wake only when ignition goes HIGH again.
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_IGNITION_SENSE, 1);
    _state = PowerState::DEEP_SLEEP;
    delay(50);   // let serial/log flush
    esp_deep_sleep_start();
    // Execution never returns past this point — reset occurs on wake.
}
