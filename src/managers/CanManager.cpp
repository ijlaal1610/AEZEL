#include "CanManager.h"
#include <driver/twai.h>
#include "Config.h"
#include "VehicleConfig.h"

static const uint8_t OBD_POLL_PIDS[] = {
    0x0C,   // Engine RPM
    0x0D,   // Vehicle Speed (km/h)
    0x05,   // Coolant Temperature
    0x11,   // Throttle Position (%)
    0x04    // Calculated Engine Load (%)
};
static const size_t NUM_OBD_PIDS = sizeof(OBD_POLL_PIDS) / sizeof(OBD_POLL_PIDS[0]);
static size_t s_pidIndex = 0;

void CanManager::begin() {
    if (!VehicleConfig::instance().usesObd()) {
        _canActive = false;
        return;
    }

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // Standard 500kbps CAN bus baud rate
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        if (twai_start() == ESP_OK) {
            _canActive = true;
        }
    }
}

void CanManager::taskEntry(void* pv) {
    CanManager& self = instance();
    if (!self._canActive && VehicleConfig::instance().usesObd()) {
        self.begin();
    }
    const TickType_t period = pdMS_TO_TICKS(50); // 20 Hz CAN tick
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void CanManager::tick() {
    if (!_canActive) {
        if (VehicleConfig::instance().usesObd()) {
            begin();
        }
        return;
    }

    // Drain all available incoming CAN frames from the RX queue
    twai_message_t message;
    while (twai_receive(&message, 0) == ESP_OK) {
        if ((message.identifier == 0x7E8 || message.identifier == 0x7EA) && message.data_length_code >= 4) {
            uint8_t pid = message.data[2];
            SharedState::instance().update([&](VehicleState& s) {
                s.obdConnected = true;

                if (pid == 0x0C && message.data_length_code >= 5) { // Engine RPM
                    uint16_t rpmVal = ((uint16_t)message.data[3] * 256 + message.data[4]) / 4;
                    s.rpm = rpmVal;
                }
                else if (pid == 0x0D && message.data_length_code >= 4) { // Vehicle Speed km/h
                    s.speedKmh = (float)message.data[3];
                }
                else if (pid == 0x05 && message.data_length_code >= 4) { // Coolant Temperature
                    float tempC = (float)((int)message.data[3] - 40);
                    s.coolantTempC = tempC;
                    s.engineTempC = tempC;
                }
                else if (pid == 0x11 && message.data_length_code >= 4) { // Throttle Position %
                    s.throttlePct = (float)message.data[3] * 100.0f / 255.0f;
                }
                else if (pid == 0x04 && message.data_length_code >= 4) { // Calculated Engine Load %
                    s.engineLoadPct = (float)message.data[3] * 100.0f / 255.0f;
                }
            });
        }
    }

    // Cyclically transmit OBD-II queries every 40ms (~5 PIDs updated every 200ms)
    uint32_t nowMs = millis();
    if (nowMs - _lastPollMs >= 40) {
        _lastPollMs = nowMs;
        twai_message_t pollMsg = {};
        pollMsg.identifier = 0x7DF; // Broadcast OBD-II Request ID
        pollMsg.data_length_code = 8;
        pollMsg.data[0] = 0x02;     // 2 additional bytes
        pollMsg.data[1] = 0x01;     // Service 01 (current data)
        pollMsg.data[2] = OBD_POLL_PIDS[s_pidIndex];
        twai_transmit(&pollMsg, pdMS_TO_TICKS(5));

        s_pidIndex = (s_pidIndex + 1) % NUM_OBD_PIDS;
    }
}
