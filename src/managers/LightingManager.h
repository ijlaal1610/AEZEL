#pragma once
// ============================================================================
//  LightingManager — drives DRL PWM, hazard relay, and the optional WS2812B
//  accent strip (welcome/goodbye animation, theme-colored ambient lighting,
//  brake-flash on hard braking). Indicator *bulbs* stay on the stock
//  motorcycle relay circuit — this module only reads their state (via
//  SensorManager's discrete inputs) to mirror it on the dash / accent ring,
//  never switches mains bulb current directly.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

class LightingManager {
public:
    static LightingManager& instance() { static LightingManager l; return l; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    void playWelcomeAnimation();
    void playGoodbyeAnimation();

private:
    LightingManager() = default;
    void updateDrlBrightness();
    void updateBrakeFlash();

    uint32_t _lastBrakeMs = 0;
    float _lastSpeedForBraking = 0;
};
