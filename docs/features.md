# AEZEL — Comprehensive Feature Specification

This document provides a detailed breakdown of every feature, subsystem, and algorithm implemented in the **AEZEL** Smart Motorcycle Cockpit and Vehicle Control Unit (VCU) platform.

---

## 📑 Table of Contents

- [1. Boot Animation & Power-Off Lifecycle](#1-boot-animation--power-off-lifecycle)
- [2. Instrumentation & LVGL Display Engine](#2-instrumentation--lvgl-display-engine)
- [3. Sensor Acquisition & Signal Processing](#3-sensor-acquisition--signal-processing)
- [4. Motorcycle Harness Sensing (Discrete Inputs)](#4-motorcycle-harness-sensing-discrete-inputs)
- [5. Ride Analytics & Trip Computer](#5-ride-analytics--trip-computer)
- [6. Security, Safety & Warning Engine](#6-security-safety--warning-engine)
- [7. Smart Lighting & Actuators](#7-smart-lighting--actuators)
- [8. Power Management & Sleep Lifecycle](#8-power-management--sleep-lifecycle)
- [9. Wireless Connectivity & Companion BLE API](#9-wireless-connectivity--companion-ble-api)
- [10. Storage & Mass Logging (NVS / SD)](#10-storage--mass-logging-nvs--sd)
- [11. FreeRTOS Dual-Core Architecture](#11-freertos-dual-core-architecture)

---

## 1. Boot Animation & Power-Off Lifecycle

AEZEL implements a fast-boot architecture targeted to be visible and responsive within **< 1.5 seconds** from ignition power-on, paired with custom hardware and UI animations.

### 1.1 Welcome / Boot Animation Sequence
- **Parallel Boot Architecture**: Heavy or high-latency tasks (GPS satellite lock, BLE advertising setup, SD card mounting) initialize in background FreeRTOS tasks on Core 0. The UI display on Core 1 initializes immediately, rendering sensible placeholder states (`"--"`, `"acquiring..."`) rather than stalling the boot sequence.
- **WS2812B NeoPixel Accent Welcome Animation**: [`LightingManager::playWelcomeAnimation()`](file:///workspaces/AEZEL/src/managers/LightingManager.cpp#L66-L75) triggers a smooth blue LED brightness pulse across the optional 24-pixel RGB accent ring on startup:
  - Fades NeoPixel brightness from `0` to `255` in increments of `15` every 20ms.
  - Illuminates pixels in electric blue (`RGB(0, 120, 255)`).
- **LVGL Theme Initialization**: Loads the active theme configuration (`MODERN_DIGITAL`, `SPORT`, `NEON`, or `RETRO`) and applies smooth 200ms screen fade-in transition (`LV_SCR_LOAD_ANIM_FADE_ON`).

### 1.2 Power-Off & Goodbye Sequence
- **LVGL Goodbye UI Screen**: [`DisplayManager::showGoodbyeScreen()`](file:///workspaces/AEZEL/src/managers/DisplayManager.cpp#L239-L249) renders a high-contrast black shutdown screen with centered white text: `"See you next ride"`.
- **WS2812B NeoPixel Goodbye Animation**: [`LightingManager::playGoodbyeAnimation()`](file:///workspaces/AEZEL/src/managers/LightingManager.cpp#L77-L86) executes a smooth fade-out:
  - Decreases brightness from `255` down to `0` in steps of `15` every 20ms.
  - Clears all accent LEDs before power shutdown.
- **Ignition Linger Delay**: When ignition is turned off, `PowerManager` enters a 5-second `LINGER` period to catch rapid ignition cycling (e.g. stalled engine restart) without triggering full shutdown.
- **Pre-Sleep Flash Protection**: Automatically flushes all dirty NVS settings (odometer, trip counters) and closes open SD card GPX/CSV files prior to deep sleep entry.

---

## 2. Instrumentation & LVGL Display Engine

The UI runs on **LVGL v8.4.0** pinned to Core 1 (`CORE_REALTIME`) at **60 FPS**.

### 2.1 Instrumentation Widgets
- **Speedometer**: Large center digital font (`lv_font_montserrat_48`), displaying vehicle speed in `km/h`.
- **Sweep Arc Tachometer**: 270-degree OEM-style sweep gauge surrounding the speedometer:
  - Range: `0` to `12,000 RPM` (headroom tailored for Avenger 150 redline).
  - Rotation: 135 degrees offset.
  - Non-draggable indicator arc with dynamic theme color accents.
- **Gear Position Indicator**: Prominent gear display centered under the speedometer (`N`, `1`, `2`, `3`, `4`, `5`).
- **Fuel Level Bar**: Vertical progress bar (`0–100%`) with animated transitions (`LV_ANIM_ON`) and green accent indicator.
- **Engine Temperature Display**: Digital readout in °C (`"--C"` when cold or acquiring).
- **Status Indicator Icon Row**: Top row featuring hidden/visible state icons:
  - Left Turn Signal (`LV_SYMBOL_LEFT`, Green)
  - Right Turn Signal (`LV_SYMBOL_RIGHT`, Green)
  - Neutral Indicator (`N`, Cyan)
  - High Beam Indicator (`LV_SYMBOL_EYE_OPEN`, Blue)
- **Real-Time Clock**: Top-right time display (`HH:MM`), throttled to update only when seconds change.
- **Odometer & Trip Computer**: Bottom display showing active Trip A distance (`A 0.0 km`) and total Odometer (`ODO 0 km`).
- **Warning Alert Banner**: Top banner overlay with red background (`#FF1744`, 80% opacity) that reveals the highest-priority active warning title.

### 2.2 UI Theme Engine
Supported color tokens per [`docs/themes.md`](file:///workspaces/AEZEL/docs/themes.md):
- `MODERN_DIGITAL`: Electric Cyan (`#00D4FF`) accents.
- `SPORT`: Flame Red (`#FF1744`) accents.
- `NEON`: Lime Green (`#39FF14`) accents.
- `RETRO`: Warm Amber (`#FF8A00`) accents.

---

## 3. Sensor Acquisition & Signal Processing

All raw hardware acquisition runs in [`SensorManager.cpp`](file:///workspaces/AEZEL/src/managers/SensorManager.cpp) at 50 Hz.

### 3.1 Pulse Sensors & Interrupts
- **Wheel Speed (Hall Sensor)**: GPIO 18 interrupt measures pulse intervals. Converts time delta into vehicle speed using configured wheel circumference (`1.518m` for 90/90-18 rear tyre) with Exponential Moving Average (EMA) noise suppression.
- **Engine RPM Pickup**: GPIO 17 interrupt attached to coil-negative / opto-isolator. Calculates engine RPM from single-cylinder pulse frequency.

### 3.2 Analog ADC Sensors (0–3.3V)
- **Fuel Float Sender**: ADC GPIO 1 reads voltage across fuel sender divider; applies exponential smoothing to prevent fuel slosh jitter.
- **Battery Voltage**: ADC GPIO 2 senses main battery voltage via 1:5 divider ratio (0–18V range).
- **Charging Line Voltage**: ADC GPIO 3 senses voltage output from the motorcycle regulator.
- **Ambient Light**: ADC GPIO 16 (or I2C BH1750) reads ambient lux level for display and DRL auto-brightness.

### 3.3 Digital & Environmental Sensors
- **1-Wire Temperature Bus**: GPIO 15 reads two Dallas DS18B20 sensors (Engine Temp & Ambient Outside Temp).
- **I2C Motion Sensing (MPU6050 IMU)**: GPIO 21 (SDA) / 20 (SCL). Calculates real-time roll lean angle, pitch angle, acceleration magnitude, and triggers crash/fall heuristics.
- **I2C Barometer (BMP280)**: Reads atmospheric pressure (hPa) and calculates barometric altitude (meters).

---

## 4. Motorcycle Harness Sensing (Discrete Inputs)

All inputs read through opto-isolators or resistor dividers on discrete GPIO pins with software debouncing:

- **Left Turn Indicator**: GPIO 19
- **Right Turn Indicator**: GPIO 0
- **Neutral Switch**: GPIO 35
- **High Beam Switch**: GPIO 36
- **Horn Switch**: GPIO 37
- **Side Stand Switch**: GPIO 41
- **Front Brake Switch**: GPIO 42
- **Rear Brake Switch**: GPIO 45
- **Clutch Switch**: GPIO 46
- **Kill Switch**: GPIO 47
- **Ignition Sense**: GPIO 48
- **Starter Switch**: GPIO 0

---

## 5. Ride Analytics & Trip Computer

Managed in [`RideManager.cpp`](file:///workspaces/AEZEL/src/managers/RideManager.cpp) at 10 Hz:

- **Trip A & Trip B**: Integrates wheel speed over time into distance (`km`). Persisted to NVS.
- **Odometer**: Cumulative lifetime vehicle distance (accumulates continuously; flushed to NVS every ~10s).
- **Ride Timer**: Elapsed time counter (seconds) for current ignition session.
- **Session Peak & Average Speed**: Tracks maximum speed (`maxSpeedKmh`) and calculates rolling average speed (`avgSpeedKmh`).
- **Fuel Range & Consumption**: Calculates fuel consumption (`km/L`) and remaining range (`km`) based on fuel level percentage and integrated trip distance.

---

## 6. Security, Safety & Warning Engine

Managed in [`NotificationManager.cpp`](file:///workspaces/AEZEL/src/managers/NotificationManager.cpp) at 4 Hz:

### 6.1 Warning Bitmask Pipeline
Monitors a 16-flag warning bitmask in `VehicleState`:
- `CHECK_ENGINE`, `OIL_PRESSURE`, `ENGINE_OVERTEMP` (Critical)
- `BATTERY_LOW`, `CHARGING_FAULT`, `FUEL_LOW`, `ABS_FAULT` (Warning)
- `CRASH_DETECTED`, `UNAUTHORIZED_MOVE` (Critical)
- `SERVICE_DUE`, `TYRE_DUE`, `CHAIN_LUBE_DUE`, `INSURANCE_EXPIRING`, `PUC_EXPIRING` (Info)
- `GPS_LOST`, `SD_CARD_FAULT` (Info)

### 6.2 Prioritized Queue & Alarms
- **Queue Management**: Stores up to 16 active notifications. When full, drops the oldest `INFO`-priority item rather than missing a new alert (`CRITICAL`/`WARNING` items are never evicted).
- **Buzzer Chimes**: Drives Piezo buzzer (GPIO 36) with distinct audible patterns:
  - `CRITICAL`: 3 rapid high-volume beeps (80ms on / 80ms off).
  - `WARNING`: 1 single beep.
  - `INFO`: Silent visual banner only.
- **Hard Braking Flasher**: [`LightingManager::updateBrakeFlash()`](file:///workspaces/AEZEL/src/managers/LightingManager.cpp#L52-L64) detects rapid deceleration (>8 km/h drop in a single tick) and flashes the RGB accent strip bright red.

---

## 7. Smart Lighting & Actuators

Managed in [`LightingManager.cpp`](file:///workspaces/AEZEL/src/managers/LightingManager.cpp):

- **Auto-Brightness DRL**: Drives Daytime Running Light PWM (GPIO 47) using 5kHz LEDC timer. Automatically scales duty cycle from 60 to 255 based on ambient light lux readings.
- **Hazard Relay**: Drives physical hazard light relay (GPIO 48) when hazard state is activated.
- **WS2812B NeoPixel Accent**: Controls 24-pixel RGB LED ring (GPIO 35) for welcome/goodbye fade animations, theme-matching ambient glow, and emergency brake flashing.

---

## 8. Power Management & Sleep Lifecycle

Managed in [`PowerManager.cpp`](file:///workspaces/AEZEL/src/managers/PowerManager.cpp):

```
[IGNITION ON]  --->  ACTIVE  ---> [IGNITION OFF] ---> LINGER (5s) ---> SAFE_SHUTDOWN ---> DEEP_SLEEP
                        ^                                    |
                        |------ [IGNITION RE-ENGAGED] -------|
```

- **Permanent Buck Supply**: Board runs on permanently-live automotive buck converter; ignition is read strictly as a logic input on GPIO 48 (`PIN_IN_IGNITION`).
- **Deep Sleep Entry**: Disables display backlight, turns off peripheral rails, flushes storage, and enters ESP32 deep sleep (`esp_deep_sleep_start()`).
- **Wake Source**: Uses `esp_sleep_enable_ext0_wakeup(PIN_IN_IGNITION, HIGH)` to wake instantly when ignition is switched on.

---

## 9. Wireless Connectivity & Companion BLE API

Managed in [`BleManager.cpp`](file:///workspaces/AEZEL/src/managers/BleManager.cpp):

- **NimBLE Stack**: Low-RAM Bluetooth Low Energy stack running on Core 0.
- **GATT Telemetry Service**: Broadcasts JSON payload over notify characteristic (`6e400002-...`) at 2 Hz:
  ```json
  {"spd":65,"rpm":5200,"fuel":82,"batt":13.8,"eng_t":88,"odo":14250,"warn":0,"lat":18.5204,"lon":73.8567}
  ```
- **Security & Bonding**: Enables MITM protection and passkey bonding for secure smartphone connection.
- **Remote Command API**: Listens on write characteristic (`6e400003-...`) for explicit allow-listed JSON commands:
  - `"reset_trip_a"`: Resets Trip A counter.
  - `"reset_trip_b"`: Resets Trip B counter.
  - `"find_bike"`: Triggers light flash + buzzer chime hook.

---

## 10. Storage & Mass Logging (NVS / SD)

Managed in [`StorageManager.cpp`](file:///workspaces/AEZEL/src/managers/StorageManager.cpp):

- **Dual-Storage Architecture**:
  - **NVS (Non-Volatile Flash)**: Stores small, critical key-values (Odometer, Trip A, Trip B, Settings, Theme preference). Flushed periodically to protect flash write endurance.
  - **SD Card (SPI Bus)**: High-capacity logging on GPIO 5 (CS), 38 (MOSI), 39 (MISO), 40 (SCK).
- **Exporter Engine**: Automatically writes telemetry log files to SD card:
  - **CSV Export**: Continuous timestamped sensor log (`timestamp, speed, rpm, fuel, temp, voltage`).
  - **GPX Export**: Standard GPS track log for mapping rides in Strava, Google Earth, or handheld GPS apps.

---

## 11. FreeRTOS Dual-Core Architecture

Configured in [`src/main.cpp`](file:///workspaces/AEZEL/src/main.cpp):

| Task Name | Priority | Core | Function |
| :--- | :---: | :---: | :--- |
| `Sensor` | 5 | Core 1 (`REALTIME`) | Hardware ADC, I2C, 1-Wire, Pulse acquisition |
| `Ride` | 5 | Core 1 (`REALTIME`) | Distance/time integration & fuel math |
| `Power` | 5 | Core 1 (`REALTIME`) | Ignition state machine & sleep logic |
| `Lighting` | 5 | Core 1 (`REALTIME`) | DRL PWM, hazard relay & WS2812B LEDs |
| `Notif` | 5 | Core 1 (`REALTIME`) | Warning flags scan & Piezo buzzer chimes |
| `Display` | 4 | Core 1 (`REALTIME`) | LVGL 8.4 UI composition & 60 FPS render loop |
| `Storage` | 2 | Core 0 (`CONNECTIVITY`) | NVS settings flush & SD CSV/GPX writes |
| `GPS` | 3 | Core 0 (`CONNECTIVITY`) | NMEA UART parsing & RTC time discipline |
| `BLE` | 3 | Core 0 (`CONNECTIVITY`) | NimBLE telemetry notify & command handler |
| `Diag` | 1 | Core 0 (`CONNECTIVITY`) | Free heap & CPU load diagnostics |

- **Watchdog Protection**: 8-second Task Watchdog (`esp_task_wdt`) monitors system health and reboots the board if any task starves the scheduler.
