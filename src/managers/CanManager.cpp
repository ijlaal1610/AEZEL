#include "CanManager.h"
#include <driver/twai.h>
#include "Config.h"

// ESP32-S3 TWAI CAN Bus configuration
#define CAN_TX_PIN GPIO_NUM_4
#define CAN_RX_PIN GPIO_NUM_5

void CanManager::begin() {
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
    if (!self._canActive) self.begin();
    const TickType_t period = pdMS_TO_TICKS(100); // 10 Hz CAN polling loop
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void CanManager::tick() {
    if (!_canActive) return;

    // Receive incoming CAN frames
    twai_message_t message;
    if (twai_receive(&message, pdMS_TO_TICKS(10)) == ESP_OK) {
        if (message.identifier == 0x7E8 && message.data_length_code >= 6) { // OBD-II ECU response
            uint8_t pid = message.data[2];
            if (pid == 0x0C) { // Engine RPM
                uint16_t rpmVal = ((uint16_t)message.data[3] * 256 + message.data[4]) / 4;
                SharedState::instance().update([=](VehicleState& s) { s.rpm = rpmVal; });
            }
            else if (pid == 0x0D) { // Vehicle Speed km/h
                uint8_t speedVal = message.data[3];
                SharedState::instance().update([=](VehicleState& s) { s.speedKmh = speedVal; });
            }
            else if (pid == 0x05) { // Engine Coolant Temp
                int tempC = (int)message.data[3] - 40;
                SharedState::instance().update([=](VehicleState& s) { s.engineTempC = tempC; });
            }
        }
    }

    // Transmit OBD-II Poll for RPM (PID 0x0C) every 200ms
    uint32_t nowMs = millis();
    if (nowMs - _lastPollMs >= 200) {
        _lastPollMs = nowMs;
        twai_message_t pollMsg = {};
        pollMsg.identifier = 0x7DF; // Broadcast OBD-II Request ID
        pollMsg.data_length_code = 8;
        pollMsg.data[0] = 0x02; // Number of bytes
        pollMsg.data[1] = 0x01; // Mode 01
        pollMsg.data[2] = 0x0C; // PID 0x0C (RPM)
        twai_transmit(&pollMsg, pdMS_TO_TICKS(10));
    }
}
