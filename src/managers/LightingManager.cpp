#include "LightingManager.h"
#include "Config.h"

// Accent strip is optional hardware, gated by ENABLE_RGB_ACCENT in Config.h —
// builds without it don't need the WS2812 dependency at all.
#if ENABLE_RGB_ACCENT
#include <Adafruit_NeoPixel.h>
static Adafruit_NeoPixel accent(24, PIN_OUT_RGB_ACCENT_DATA, NEO_GRB + NEO_KHZ800);
#endif

void LightingManager::begin() {
    pinMode(PIN_OUT_DRL_PWM, OUTPUT);
    pinMode(PIN_OUT_HAZARD_RELAY, OUTPUT);
    ledcSetup(0, 5000, 8);              // channel 0, 5kHz, 8-bit
    ledcAttachPin(PIN_OUT_DRL_PWM, 0);

#if ENABLE_RGB_ACCENT
    accent.begin();
    accent.setBrightness(80);
    accent.show();
#endif
}

void LightingManager::taskEntry(void* pv) {
    LightingManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(50);
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void LightingManager::tick() {
    updateDrlBrightness();
    updateBrakeFlash();

    VehicleState s = SharedState::instance().snapshot();
    digitalWrite(PIN_OUT_HAZARD_RELAY, s.inHazard ? HIGH : LOW);
}

void LightingManager::updateDrlBrightness() {
    VehicleState s = SharedState::instance().snapshot();
    // Auto-brightness: map ambient lux to DRL PWM duty. Replace lightLux
    // source with a real BH1750 reading (Config.h I2C bus) — currently
    // SensorManager leaves lightLux at 0 until that sensor is wired in.
    uint8_t duty = s.lightLux > 500 ? 255 : map((int)s.lightLux, 0, 500, 60, 255);
    ledcWrite(0, duty);
}

void LightingManager::updateBrakeFlash() {
    VehicleState s = SharedState::instance().snapshot();
    bool hardBraking = (_lastSpeedForBraking - s.speedKmh) > 8.0f;   // >8km/h drop this tick
    _lastSpeedForBraking = s.speedKmh;

#if ENABLE_RGB_ACCENT
    if (hardBraking && millis() - _lastBrakeMs > 200) {
        _lastBrakeMs = millis();
        for (int i = 0; i < accent.numPixels(); i++) accent.setPixelColor(i, accent.Color(255, 0, 0));
        accent.show();
    }
#endif
}

void LightingManager::playWelcomeAnimation() {
#if ENABLE_RGB_ACCENT
    for (int b = 0; b < 255; b += 15) {
        accent.setBrightness(b);
        for (int i = 0; i < accent.numPixels(); i++) accent.setPixelColor(i, accent.Color(0, 120, 255));
        accent.show();
        delay(20);
    }
#endif
}

void LightingManager::playGoodbyeAnimation() {
#if ENABLE_RGB_ACCENT
    for (int b = 255; b > 0; b -= 15) {
        accent.setBrightness(b);
        accent.show();
        delay(20);
    }
    accent.clear();
    accent.show();
#endif
}
