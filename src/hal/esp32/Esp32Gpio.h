#pragma once
// ============================================================================
//  Esp32Gpio.h — ESP32-S3 Hardware GPIO & LEDC implementation
// ============================================================================
#if defined(ESP32) || defined(ARDUINO)
#include "hal/IHalGpio.h"
#include <Arduino.h>

class Esp32Gpio : public IHalGpio {
public:
    Esp32Gpio() = default;
    virtual ~Esp32Gpio() override = default;

    virtual void setPinMode(uint8_t pin, GpioMode mode) override {
        switch (mode) {
            case GpioMode::INPUT:          pinMode(pin, INPUT); break;
            case GpioMode::OUTPUT:         pinMode(pin, OUTPUT); break;
            case GpioMode::INPUT_PULLUP:   pinMode(pin, INPUT_PULLUP); break;
            case GpioMode::INPUT_PULLDOWN: pinMode(pin, INPUT_PULLDOWN); break;
        }
    }

    virtual void writePin(uint8_t pin, PinLevel level) override {
        digitalWrite(pin, level == PinLevel::HIGH_VAL ? HIGH : LOW);
    }

    virtual PinLevel readPin(uint8_t pin) override {
        return digitalRead(pin) == HIGH ? PinLevel::HIGH_VAL : PinLevel::LOW_VAL;
    }

    virtual void setPwm(uint8_t channel, uint8_t pin, uint32_t freqHz, uint8_t duty) override {
        ledcSetup(channel, freqHz, 8);
        ledcAttachPin(pin, channel);
        ledcWrite(channel, duty);
    }

    virtual void writePwm(uint8_t channel, uint8_t duty) override {
        ledcWrite(channel, duty);
    }

    virtual bool attachInterruptHandler(uint8_t pin, std::function<void()> isr, InterruptTrigger trigger) override {
        (void)pin;
        (void)isr;
        (void)trigger;
        return true;
    }
};
#endif
