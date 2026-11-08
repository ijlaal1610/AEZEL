#pragma once
// ============================================================================
//  Esp32Can.h — ESP32-S3 TWAI CAN driver implementation
// ============================================================================
#if defined(ESP32) || defined(ARDUINO)
#include "hal/IHalCan.h"
#include <driver/twai.h>
#include <cstring>

class Esp32Can : public IHalCan {
public:
    Esp32Can() = default;
    virtual ~Esp32Can() override { stop(); }

    virtual bool begin(uint32_t baudRate, int8_t rxPin, int8_t txPin) override {
        (void)baudRate;
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
            (gpio_num_t)txPin, (gpio_num_t)rxPin, TWAI_MODE_NORMAL);
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
            return false;
        }
        if (twai_start() != ESP_OK) {
            twai_driver_uninstall();
            return false;
        }
        _installed = true;
        return true;
    }

    virtual bool sendFrame(const CanFrame& frame) override {
        if (!_installed) return false;
        twai_message_t tx_msg;
        std::memset(&tx_msg, 0, sizeof(tx_msg));
        tx_msg.identifier = frame.id;
        tx_msg.extd = frame.extended ? 1 : 0;
        tx_msg.data_length_code = frame.len > 8 ? 8 : frame.len;
        std::memcpy(tx_msg.data, frame.data, tx_msg.data_length_code);
        return twai_transmit(&tx_msg, pdMS_TO_TICKS(50)) == ESP_OK;
    }

    virtual bool receiveFrame(CanFrame& frame, uint32_t timeoutMs) override {
        if (!_installed) return false;
        twai_message_t rx_msg;
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(timeoutMs)) != ESP_OK) {
            return false;
        }
        frame.id = rx_msg.identifier;
        frame.extended = rx_msg.extd != 0;
        frame.len = rx_msg.data_length_code;
        std::memcpy(frame.data, rx_msg.data, frame.len);
        return true;
    }

    virtual bool isConnected() const override { return _installed; }

    virtual void stop() override {
        if (_installed) {
            twai_stop();
            twai_driver_uninstall();
            _installed = false;
        }
    }

private:
    bool _installed = false;
};
#endif
