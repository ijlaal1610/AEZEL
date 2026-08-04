#pragma once
// ============================================================================
//  Config.h — Central pin map & compile-time constants
//  Change hardware wiring ONLY here. No pin numbers anywhere else in code.
// ============================================================================
#include <Arduino.h>

// ============================================================================
//  FEATURE FLAGS — build with only the hardware you actually have installed.
//
//  Every manager checks the matching flag in its begin()/tick() before
//  touching its pins/bus, so leaving a flag OFF means that manager's begin()
//  becomes a harmless no-op and its task is never even created in main.cpp —
//  no crashes, no "sensor not found" spam, no wasted RAM/CPU on a peripheral
//  that isn't physically wired up yet. See docs/incremental_build.md for the
//  recommended purchase order that pairs with these flags.
// ============================================================================
#define ENABLE_GPS            0   // NEO-6M/8M module — Tier 3
#define ENABLE_IMU             0   // MPU6050/ICM — Tier 4 (lean angle, crash detect)
#define ENABLE_BAROMETER        0   // BMP280 — Tier 4 (altitude, pressure)
#define ENABLE_AMBIENT_LIGHT     0   // BH1750 — Tier 2 (auto-brightness; falls back to fixed brightness when off)
#define ENABLE_SD_CARD             1   // Tier 1 — cheap and worth having from day one for ride logs
#define ENABLE_RGB_ACCENT           0   // WS2812 strip — Tier 4, cosmetic only
#define ENABLE_BLE                   1   // built into every ESP32 — no extra hardware, keep on
#define ENABLE_ONEWIRE_TEMP            0   // DS18B20 x2 — Tier 2
#define ENABLE_FUEL_SENDER               0   // Tier 2 — needs signal-conditioning board
#define ENABLE_TOUCHSCREEN                 0   // Tier 1 optional — UI works with rotary/buttons alone

// --- Remote-control sub-features (all gated behind ENABLE_BLE) -----------
// See docs/remote_control.md before enabling anything in this block — the
// starter/ignition ones are NOT "flip it on and go," they require the
// physical interlock wiring described there.
#define ENABLE_REMOTE_HORN               0   // Tier 1.5 — one relay, low risk
#define ENABLE_REMOTE_INDICATORS          0   // Tier 1.5 — one relay per side, low risk
#define ENABLE_REMOTE_IMMOBILIZER           0   // Tier 4 — cuts ignition circuit; fail-safe = unlocked
#define ENABLE_REMOTE_STARTER                 0   // Tier 5 — highest risk, see docs/remote_control.md. Off by default.

// ---------------------------------------------------------------- Display --
// Parallel/SPI TFT wired per TFT_eSPI User_Setup.h (kept in sync with these).
#define PIN_TFT_BL          9      // backlight PWM (also drives auto-brightness)
#define PIN_TFT_CS          10
#define PIN_TFT_DC          11
#define PIN_TFT_RST         12
#define PIN_TOUCH_CS        13
#define PIN_TOUCH_IRQ       14

// ------------------------------------------------------------- Input HW ---
#define PIN_ROTARY_A        4
#define PIN_ROTARY_B        5
#define PIN_ROTARY_SW       6
#define PIN_BTN_MODE        7      // physical mode/back button
#define PIN_BTN_OK          8

// -------------------------------------------------------------- Sensors ---
#define PIN_SPEED_HALL       18    // wheel-speed hall sensor (interrupt, pull-up)
#define PIN_RPM_PICKUP       17    // RPM pickup / coil-negative via opto-isolator (interrupt)
#define PIN_FUEL_SENDER_ADC  1     // analog fuel float sender (via divider, 0-3.3V)
#define PIN_BATTERY_ADC      2     // battery voltage sense (via 1:5 divider -> max ~18V)
#define PIN_CHARGE_ADC       3     // charging-line voltage sense (regulator output)
#define PIN_ONEWIRE_BUS      15    // DS18B20 x2 (engine + ambient) on one bus
#define PIN_LIGHT_SENSOR_ADC 16    // ambient light (LDR or BH1750 via I2C instead - see below)
#define I2C_SDA              21    // shared bus: IMU, RTC, BMP280, ambient light (BH1750)
#define I2C_SCL              22

// ------------------------------------------------------- Indicator inputs -
// All read from existing motorcycle harness through opto-isolators / voltage
// dividers - NEVER connect 12V direct to ESP32 GPIO.
#define PIN_IN_LEFT_INDICATOR   33
#define PIN_IN_RIGHT_INDICATOR  34
#define PIN_IN_NEUTRAL          35
#define PIN_IN_HIGH_BEAM        36
#define PIN_IN_HORN_SW          37
#define PIN_IN_SIDE_STAND       38
#define PIN_IN_FRONT_BRAKE      39
#define PIN_IN_REAR_BRAKE       40
#define PIN_IN_CLUTCH           41
#define PIN_IN_KILL_SWITCH      42
#define PIN_IN_IGNITION         45   // wake source, see PowerManager
#define PIN_IN_STARTER          46

// ------------------------------------------------------ Output / lighting -
#define PIN_OUT_DRL_PWM         47
#define PIN_OUT_HAZARD_RELAY    48
#define PIN_OUT_RGB_ACCENT_DATA 26   // WS2812B accent strip (welcome/goodbye/theme)
#define PIN_OUT_BUZZER          27

// ------------------------------------------------- Remote-control outputs -
// Every one of these drives a relay/MOSFET, never the load directly — see
// docs/remote_control.md for the driver-stage requirements per output.
#define PIN_OUT_HORN_RELAY        28   // ENABLE_REMOTE_HORN
#define PIN_OUT_LEFT_IND_RELAY    29   // ENABLE_REMOTE_INDICATORS (drives indicator circuit in parallel with the stock switch)
#define PIN_OUT_RIGHT_IND_RELAY   30   // ENABLE_REMOTE_INDICATORS
#define PIN_OUT_IMMOBILIZER_RELAY 31   // ENABLE_REMOTE_IMMOBILIZER — normally-closed relay in the ignition-coil-enable circuit; see docs/remote_control.md fail-safe requirement
#define PIN_OUT_STARTER_RELAY     32   // ENABLE_REMOTE_STARTER — see docs/remote_control.md interlock requirements before wiring

// ------------------------------------------------------------- Storage ---
#define PIN_SD_CS            5   // shares SPI bus with TFT (separate CS)
#define PIN_SD_MOSI          38
#define PIN_SD_MISO          39
#define PIN_SD_SCK           40

// --------------------------------------------------------------- GPS -----
#define GPS_UART_NUM         1
#define PIN_GPS_RX           44
#define PIN_GPS_TX           43
#define GPS_BAUD              9600

// ---------------------------------------------------------- Power mgmt ---
#define PIN_IGNITION_SENSE   PIN_IN_IGNITION   // also EXT0 deep-sleep wake source
#define PIN_5V_ENABLE        2                  // enables buck converter's 5V rail relay (optional)

// ----------------------------------------------------- Derived constants -
// Wheel circumference for a stock 90/90-18 rear tyre - recalibrate in
// Settings > Calibration Wizard after fitting non-stock tyres.
constexpr float WHEEL_CIRCUMFERENCE_M   = 1.518f;
// 4 magnets equally spaced around the wheel/disc (or reuse an existing ABS
// reluctor ring's tooth count if fitted). A single magnet (1 pulse/rev)
// sounds simpler but doesn't give enough pulses per calculation window at
// normal riding speed for a stable reading — this was caught by
// test/native/test_vehicle_math.cpp's regression test, see that file's
// comments for the numbers. Update this to match however many magnets you
// actually install.
constexpr uint8_t HALL_PULSES_PER_REV   = 4;
constexpr uint8_t RPM_PICKUP_PULSES_PER_REV = 1;   // single-cylinder, 1 pulse/rev on coil-negative

// Voltage-divider ratios (must match your resistor values on the board)
constexpr float BATTERY_DIVIDER_RATIO   = 5.0f;    // e.g. 40k/10k -> Vbat = Vadc * 5
constexpr float CHARGE_DIVIDER_RATIO    = 5.0f;

// Display
constexpr uint16_t SCREEN_W = 480;
constexpr uint16_t SCREEN_H = 320;
constexpr uint8_t  TARGET_FPS = 60;

// FreeRTOS task priorities (higher = more urgent). Keep sensor/safety tasks
// above UI so a slow render frame never delays a crash-detect or warning.
enum TaskPriority : UBaseType_t {
    PRIO_SAFETY_MONITOR = 6,
    PRIO_SENSOR         = 5,
    PRIO_POWER          = 5,
    PRIO_GPS            = 3,
    PRIO_BLE            = 3,
    PRIO_STORAGE        = 2,
    PRIO_DISPLAY        = 4,
    PRIO_DIAGNOSTICS    = 1,
};

// Core pinning: Core 0 = connectivity/background, Core 1 = UI + time-critical
constexpr BaseType_t CORE_CONNECTIVITY = 0;
constexpr BaseType_t CORE_REALTIME     = 1;
