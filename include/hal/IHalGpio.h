#pragma once
// ============================================================================
//  IHalGpio.h — Hardware Abstraction Layer for GPIO & PWM
//  Compatible with ESP32-S3 (Arduino/ESP-IDF) and Raspberry Pi (Linux/libgpiod)
// ============================================================================
#include <stdint.h>
#include <functional>

enum class GpioMode : uint8_t {
    INPUT,
    OUTPUT,
    INPUT_PULLUP,
    INPUT_PULLDOWN
};

enum class PinLevel : uint8_t {
    LOW_VAL = 0,
    HIGH_VAL = 1
};

enum class InterruptTrigger : uint8_t {
    RISING_EDGE,
    FALLING_EDGE,
    BOTH_EDGES
};

class IHalGpio {
public:
    virtual ~IHalGpio() = default;
    virtual void setPinMode(uint8_t pin, GpioMode mode) = 0;
    virtual void writePin(uint8_t pin, PinLevel level) = 0;
    virtual PinLevel readPin(uint8_t pin) = 0;
    virtual void setPwm(uint8_t channel, uint8_t pin, uint32_t freqHz, uint8_t duty) = 0;
    virtual void writePwm(uint8_t channel, uint8_t duty) = 0;
    virtual bool attachInterruptHandler(uint8_t pin, std::function<void()> isr, InterruptTrigger trigger) = 0;
};
