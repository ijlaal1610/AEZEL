#pragma once
// ============================================================================
//  SensorManager — owns all raw sensor acquisition.
//  Runs as its own FreeRTOS task at fixed 20 Hz; interrupt-driven pulse
//  counters (speed/RPM) are ISR-safe and integrated on each tick.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

class SensorManager {
public:
    static SensorManager& instance() { static SensorManager s; return s; }

    void begin();                      // configure pins, interrupts, I2C sensors
    static void taskEntry(void* pv);   // FreeRTOS task trampoline
    void tick();                       // one 50ms sample/compute cycle

    // ISR handlers (must be static, IRAM_ATTR)
    static void IRAM_ATTR isrSpeedPulse();
    static void IRAM_ATTR isrRpmPulse();

private:
    SensorManager() = default;

    void readAnalogChannels();
    void computeSpeed(float dtSec);
    void computeRpm(float dtSec);
    void readOneWireTemps();
    void readImu();
    void readEnvironmental();
    void updateIndicatorInputs();
    void evaluateWarnings();

    volatile static uint32_t _speedPulseCount;
    volatile static uint32_t _rpmPulseCount;

    // EMA smoothing to avoid jittery needle/digit updates from a single
    // noisy sample — tune alpha per channel.
    float _speedEma = 0, _rpmEma = 0, _fuelEma = 0;
    uint32_t _lastTickMs = 0;

    bool _imuOk = false, _envOk = false;
};
