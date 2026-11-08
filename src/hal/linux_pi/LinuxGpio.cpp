#include "LinuxGpio.h"
#include <iostream>

void LinuxGpio::setPinMode(uint8_t pin, GpioMode mode) {
    std::lock_guard<std::mutex> lock(_mutex);
    _pinModes[pin] = mode;
}

void LinuxGpio::writePin(uint8_t pin, PinLevel level) {
    std::lock_guard<std::mutex> lock(_mutex);
    _pinStates[pin] = level;
}

PinLevel LinuxGpio::readPin(uint8_t pin) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _pinStates.find(pin);
    if (it != _pinStates.end()) {
        return it->second;
    }
    return PinLevel::LOW_VAL;
}

void LinuxGpio::setPwm(uint8_t channel, uint8_t pin, uint32_t freqHz, uint8_t duty) {
    std::lock_guard<std::mutex> lock(_mutex);
    (void)pin;
    (void)freqHz;
    _pwmDuty[channel] = duty;
}

void LinuxGpio::writePwm(uint8_t channel, uint8_t duty) {
    std::lock_guard<std::mutex> lock(_mutex);
    _pwmDuty[channel] = duty;
}

bool LinuxGpio::attachInterruptHandler(uint8_t pin, std::function<void()> isr, InterruptTrigger trigger) {
    (void)pin;
    (void)isr;
    (void)trigger;
    // On Linux/RPi, can be wired to libgpiod edge events or sysfs epoll
    return true;
}
