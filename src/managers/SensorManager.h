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

    // Speed/RPM are computed over a WIDER window than the 50ms task tick —
    // see the comment in SensorManager.cpp::tick(). A 1-pulse-per-revolution
    // wheel sensor produces under 1 pulse per 50ms at normal riding speeds,
    // so counting pulses in a 50ms window quantizes to 0-or-1 and reads
    // wildly noisy. Accumulating over ~200ms fixes it without meaningfully
    // hurting dashboard responsiveness (5Hz updates are imperceptible on a
    // speedometer). This was caught by test/native/test_vehicle_math.cpp's
    // regression test — see that file's comments for the numbers.
    uint32_t _lastSpeedRpmCalcMs = 0;
    static constexpr uint32_t SPEED_RPM_CALC_WINDOW_MS = 200;

    bool _imuOk = false, _envOk = false;
};
