#pragma once
// ============================================================================
//  LinuxGpio.h — Linux / Raspberry Pi GPIO implementation
// ============================================================================
#include "hal/IHalGpio.h"
#include <map>
#include <mutex>

class LinuxGpio : public IHalGpio {
public:
    LinuxGpio() = default;
    virtual ~LinuxGpio() override = default;

    virtual void setPinMode(uint8_t pin, GpioMode mode) override;
    virtual void writePin(uint8_t pin, PinLevel level) override;
    virtual PinLevel readPin(uint8_t pin) override;
    virtual void setPwm(uint8_t channel, uint8_t pin, uint32_t freqHz, uint8_t duty) override;
    virtual void writePwm(uint8_t channel, uint8_t duty) override;
    virtual bool attachInterruptHandler(uint8_t pin, std::function<void()> isr, InterruptTrigger trigger) override;

private:
    std::mutex _mutex;
    std::map<uint8_t, GpioMode> _pinModes;
    std::map<uint8_t, PinLevel> _pinStates;
    std::map<uint8_t, uint8_t> _pwmDuty;
};
