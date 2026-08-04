#pragma once
// ============================================================================
//  Config.h — Central pin map & compile-time constants
//  Change hardware wiring ONLY here. No pin numbers anywhere else in code.
// ============================================================================
#include <Arduino.h>

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
constexpr uint8_t HALL_PULSES_PER_REV   = 1;
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
