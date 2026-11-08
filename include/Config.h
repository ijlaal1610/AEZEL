#pragma once
// ============================================================================
//  Config.h — Central pin map, feature flags & compile-time constants
//  Change hardware wiring ONLY here. No pin numbers anywhere else in code.
// ============================================================================
#include <Arduino.h>

// ----------------------------------------------------------------------------
// Compile-Time Subsystem Feature Flags
// ----------------------------------------------------------------------------
#define ENABLE_CAN_BUS              1   // Automotive TWAI / OBD-II telemetry
#define ENABLE_OTA_WIFI             1   // Wireless AP & HTTP firmware updates
#define OTA_WIFI_AP_SSID            "AEZEL-VCU-AP"
#define OTA_WIFI_AP_PASS            "aezel2026"
#define ENABLE_NAVIGATION           1   // Turn-by-turn navigation engine
#define ENABLE_GPS                  1   // NEO-6M / NEO-8M UART GPS receiver
#define ENABLE_IMU                  1   // MPU6050 6-DOF lean angle & crash detection
#define ENABLE_BAROMETER            1   // BMP280 barometric altitude / pressure
#define ENABLE_SD_CARD              1   // MicroSD SPI ride logging & GPX export
#define ENABLE_BLE                  1   // NimBLE telemetry & smartphone sync
#define ENABLE_TOUCHSCREEN          1   // Capacitive/resistive display touch
#define ENABLE_RGB_ACCENT           1   // WS2812B multi-color accent strip
#define ENABLE_ONEWIRE_TEMP         1   // DS18B20 engine & ambient temperature
#define ENABLE_FUEL_SENDER          1   // Analog fuel float sender ADC
#define ENABLE_REMOTE_STARTER       1   // Relayed remote engine start
#define ENABLE_REMOTE_HORN          1   // Relayed remote horn chirp
#define ENABLE_REMOTE_INDICATORS    1   // Relayed remote hazard/indicator flashing
#define ENABLE_REMOTE_IMMOBILIZER   1   // Relayed anti-theft ignition cut

// ---------------------------------------------------------------- Display --
// Parallel/SPI TFT wired per TFT_eSPI User_Setup.h (kept in sync with these).
#define PIN_TFT_BL                  9   // Backlight PWM (LEDC Channel 0)
#define PIN_TFT_CS                  10
#define PIN_TFT_DC                  11
#define PIN_TFT_RST                 12
#define PIN_TOUCH_CS                13
#define PIN_TOUCH_IRQ               14

// ------------------------------------------------------------- Storage ---
#define PIN_SD_CS                   5   // MicroSD Chip Select (shares SPI bus with TFT)
#define PIN_SD_MOSI                 38  // Hardware SPI MOSI (Dedicated to SPI Bus)
#define PIN_SD_MISO                 39  // Hardware SPI MISO (Dedicated to SPI Bus)
#define PIN_SD_SCK                  40  // Hardware SPI SCK  (Dedicated to SPI Bus)

// ------------------------------------------------------------- I2C Bus ---
// Shared bus: DS3231 RTC, MPU6050 IMU, BMP280 Barometer, BH1750 Ambient Light
#define I2C_SDA                     15
#define I2C_SCL                     16

// -------------------------------------------------------------- Sensors ---
#define PIN_FUEL_SENDER_ADC         1   // Analog fuel float sender (ADC1_CH0, 0-3.3V)
#define PIN_BATTERY_ADC             2   // Battery voltage divider (ADC1_CH1, 1:5 ratio)
#define PIN_CHARGE_ADC              3   // Charging line voltage (ADC1_CH2)
#define PIN_ONEWIRE_BUS             21  // DS18B20 digital temperature bus
#define PIN_SPEED_HALL              7   // Wheel-speed hall sensor (interrupt, pull-up)
#define PIN_RPM_PICKUP              8   // Coil-negative / spark opto-isolator (interrupt)

// ---------------------------------------------------------- Power mgmt ---
#define PIN_IN_IGNITION             4   // RTC GPIO 4 — EXT0 deep sleep wakeup source!
#define PIN_IGNITION_SENSE          PIN_IN_IGNITION
#define PIN_5V_ENABLE               6   // Active-high buck converter power rail relay

// ------------------------------------------------------- Indicator Inputs -
// Read from vehicle harness through opto-isolators (3.3V logic level)
#define PIN_IN_LEFT_INDICATOR       33
#define PIN_IN_RIGHT_INDICATOR      34
#define PIN_IN_NEUTRAL              35
#define PIN_IN_HIGH_BEAM            36
#define PIN_IN_HORN_SW              37
#define PIN_IN_SIDE_STAND           41  // Reassigned away from SPI MOSI (38)
#define PIN_IN_FRONT_BRAKE          42  // Reassigned away from SPI MISO (39)
#define PIN_IN_REAR_BRAKE           45  // Reassigned away from SPI SCK  (40)
#define PIN_IN_CLUTCH               46
#define PIN_IN_KILL_SWITCH          17

// ------------------------------------------------------ Output / Relays --
#define PIN_OUT_STARTER_RELAY       19  // 12V starter solenoid pulse relay
#define PIN_OUT_IGNITION_RELAY      20  // 12V ignition circuit relay
#define PIN_OUT_RGB_ACCENT_DATA     26  // WS2812B NeoPixel accent strip
#define PIN_OUT_BUZZER              27  // Piezo notification buzzer
#define PIN_OUT_HORN_RELAY          28  // 12V vehicle horn relay (independent from buzzer)
#define PIN_OUT_HAZARD_RELAY        48  // 12V hazard light relay
#define PIN_OUT_DRL_PWM             47  // Daytime Running Light PWM (LEDC Channel 1)
#define PIN_OUT_LEFT_IND_RELAY      31  // Left turn indicator relay
#define PIN_OUT_RIGHT_IND_RELAY     32  // Right turn indicator relay
#define PIN_OUT_IMMOBILIZER_RELAY   18  // Anti-theft starter/ignition cut relay

// --------------------------------------------------------------- GPS -----
#define GPS_UART_NUM                1
#define PIN_GPS_TX                  43  // Dedicated GPS UART TX
#define PIN_GPS_RX                  44  // Dedicated GPS UART RX
#define GPS_BAUD                    9600

// ----------------------------------------------------------------- CAN ---
// Automotive TWAI / OBD-II Transceiver (MCP2551 / SN65HVD230)
#define CAN_TX_PIN                  GPIO_NUM_18 // Dedicated TWAI TX (moved away from GPS 43)
#define CAN_RX_PIN                  GPIO_NUM_19 // Dedicated TWAI RX (moved away from GPS 44)

// ----------------------------------------------------- Display Specs ----
constexpr uint16_t SCREEN_W         = 480;
constexpr uint16_t SCREEN_H         = 320;
constexpr uint8_t  TARGET_FPS       = 60;

// LEDC PWM Channel Allocation
constexpr uint8_t LEDC_CHANNEL_BACKLIGHT = 0;
constexpr uint8_t LEDC_CHANNEL_DRL       = 1;

// ----------------------------------------------------- FreeRTOS Cores ---
constexpr BaseType_t CORE_CONNECTIVITY = 0; // BLE, GPS, CAN, WiFi OTA, Storage
constexpr BaseType_t CORE_REALTIME     = 1; // UI, Sensors, Relays, Power, Notifs

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
