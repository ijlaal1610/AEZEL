# AEZEL — Exhaustive Features & Technical Deep Dive

This document provides an exhaustive, low-level technical reference for every single feature, algorithm, hardware interface, and mathematical formula implemented in the **AEZEL** Smart Motorcycle Cockpit and Vehicle Control Unit (VCU) platform.

---

## 📑 Table of Contents

- [1. Executive Architecture Summary](#1-executive-architecture-summary)
- [2. Detailed Subsystem Features](#2-detailed-subsystem-features)
  - [2.1 Instrumentation & Display Subsystem](#21-instrumentation--display-subsystem)
  - [2.2 Sensor Acquisition & DSP Math](#22-sensor-acquisition--dsp-math)
  - [2.3 Harness Input Sensing (Opto-Isolated Discrete Inputs)](#23-harness-input-sensing-opto-isolated-discrete-inputs)
  - [2.4 Ride Computer & Analytics Integration](#24-ride-computer--analytics-integration)
  - [2.5 Power Management & Deep Sleep Lifecycle](#25-power-management--deep-sleep-lifecycle)
  - [2.6 Safety, Warnings & Buzzer Engine](#26-safety-warnings--buzzer-engine)
  - [2.7 Smart Lighting & Actuators](#27-smart-lighting--actuators)
  - [2.8 GPS Navigation & Time Discipline](#28-gps-navigation--time-discipline)
  - [2.9 Wireless Connectivity & NimBLE Telemetry](#29-wireless-connectivity--nimble-telemetry)
  - [2.10 Storage & Mass Logging (NVS / SD)](#210-storage--mass-logging-nvs--sd)
- [3. FreeRTOS Scheduling & Core Allocation](#3-freertos-scheduling--core-allocation)

---

## 1. Executive Architecture Summary

AEZEL uses a **mutex-guarded single source of truth** ([`SharedState`](file:///workspaces/AEZEL/include/DataModel.h)) for all live vehicle telemetry. Subsystem managers never access raw global variables or reach into each other's memory space. 

Execution is split across the ESP32-S3's dual Xtensa cores:
- **Core 1 (`CORE_REALTIME`)**: Time-critical UI rendering (60 FPS), sensor acquisition (50 Hz), safety checks, and lighting PWM.
- **Core 0 (`CORE_CONNECTIVITY`)**: Background I/O (GPS UART, NimBLE stack, SD card log flushes, system diagnostics).

---

## 2. Detailed Subsystem Features

### 2.1 Instrumentation & Display Subsystem

#### A. 3-Phase OEM Boot Animation & Self-Test
- **Phase 1: Splash Logo Reveal (0 – 500 ms)**:
  - Renders `_screenSplash` with high-contrast background (`#07090E`).
  - Displays stylized brand title `"AEZEL"` in 48 pt typography (`lv_font_montserrat_48`).
  - Subtitle `"VCU SYSTEM CHECK..."` with an animated LVGL progress bar (`_barBootProgress`) filling `0% → 100%`.
  - WS2812B NeoPixel accent ring executes a 360° rotational blue welcome chase pulse (`RGB(0, 120, 255)`).
- **Phase 2: Tachometer Gauge Sweep & Bulb Self-Test (500 ms – 1200 ms)**:
  - Fades smoothly from Splash screen to Main Dashboard (`LV_SCR_LOAD_ANIM_FADE_ON`).
  - **Tachometer Arc Sweep**: RPM arc gauge smoothly sweeps `0 → 12,000 RPM → 0` over 700 ms.
  - **Speedometer Segment Test**: Digital speedometer displays `"188"` segment self-test.
  - **Indicator Bulb Self-Test**: Simultaneously illuminates Left Turn, Right Turn, Neutral, and High Beam icons for visual confirmation.
- **Phase 3: Live Telemetry Handover (1200 ms+)**:
  - Indicators return to real hardware inputs, needles return to live sensor readings, and 60 FPS live telemetry rendering begins.

#### B. LVGL 8.4 Dashboard Widgets
- **Speedometer**: Center digital display (`km/h`) using `lv_font_montserrat_48`.
- **Tachometer Arc**: 270-degree sweep arc gauge (0 to 12,000 RPM) with dynamic theme accent colors (`LV_PART_INDICATOR`).
- **Gear Position Indicator**: Center bottom font displaying `N`, `1`, `2`, `3`, `4`, `5`.
- **Fuel Level Bar**: Vertical progress bar (`0–100%`) with smooth animation transitions (`LV_ANIM_ON`).
- **Engine Temperature**: Digital readout in °C (`"--C"` when uninitialized).
- **Indicator Bar**: Icons for Left Turn (`LV_SYMBOL_LEFT`), Right Turn (`LV_SYMBOL_RIGHT`), Neutral (`N`), and High Beam (`LV_SYMBOL_EYE_OPEN`).
- **Clock**: `HH:MM` display synchronized with RTC/GPS time.
- **Trip/Odometer**: Bottom status bar displaying active Trip A (`A 0.0 km`) and total Odometer (`ODO 0 km`).
- **Warning Alert Banner**: Top overlay banner (`#FF1744`, 80% opacity) displaying the highest-priority active warning title.

#### C. Theme Engine
Supports live theme token swapping:
- `MODERN_DIGITAL`: Electric Cyan (`#00D4FF`)
- `SPORT`: Flame Red (`#FF1744`)
- `NEON`: Lime Green (`#39FF14`)
- `RETRO`: Warm Amber (`#FF8A00`)

#### D. Power-Off Goodbye Screen
- Synchronous black shutdown screen with centered white text `"See you next ride"`.
- NeoPixel ring fade-out animation from brightness `255` down to `0` over 350 ms.

---

### 2.2 Sensor Acquisition & DSP Math

#### A. Wheel Speed Processing
- **Hardware**: Hall effect pulse sensor on rear wheel (GPIO 18, interrupt driven).
- **Formula**:
  $$\text{speed}_{\text{km/h}} = \left( \frac{\text{WHEEL\_CIRCUMFERENCE\_M}}{\Delta t_{\text{seconds}}} \right) \times 3.6$$
  where $\text{WHEEL\_CIRCUMFERENCE\_M} = 1.518\text{ m}$ (stock 90/90-18 rear tyre).
- **Noise Suppression**: Filtered using an Exponential Moving Average (EMA):
  $$\text{speed}_{\text{filtered}} = \alpha \cdot \text{speed}_{\text{raw}} + (1 - \alpha) \cdot \text{speed}_{\text{prev}}, \quad \alpha = 0.25$$

#### B. Engine RPM Pickup
- **Hardware**: Ignition coil-negative / opto-isolator pickup (GPIO 17, interrupt driven).
- **Formula**:
  $$\text{RPM} = \left( \frac{\text{Pulses}}{\Delta t_{\text{seconds}}} \right) \times 60$$
  Tailored for single-cylinder 4-stroke engines (1 pulse per revolution on coil negative).

#### C. Analog Fuel Sender ADC
- **Hardware**: Fuel float resistive sender on GPIO 1 (0–3.3V ADC).
- **Filtering**: 16-sample exponential smoothing to eliminate fuel slosh jitter during acceleration/braking.

#### D. Electrical Voltage Monitoring
- **Hardware**: Battery Voltage (GPIO 2) & Charging Line Voltage (GPIO 3) via 1:5 resistive dividers (40kΩ / 10kΩ).
- **Formula**:
  $$V_{\text{actual}} = V_{\text{ADC}} \times 5.0$$

#### E. 1-Wire Temperature Bus
- **Hardware**: Dual Dallas DS18B20 sensors on GPIO 15 (Engine Temp & Outside Ambient Temp).

#### F. 6-Axis Motion & Attitude Math (MPU6050 IMU)
- **Hardware**: I2C bus (GPIO 21 SDA / GPIO 20 SCL).
- **Lean Angle (Roll)**:
  $$\theta_{\text{roll}} = \arctan2(a_y, a_z) \times \frac{180}{\pi}$$
- **Pitch Angle**:
  $$\theta_{\text{pitch}} = \arctan2(-a_x, \sqrt{a_y^2 + a_z^2}) \times \frac{180}{\pi}$$
- **Crash Detection Heuristic**: Triggers when vector acceleration exceeds $3.5g$ or roll angle exceeds $65^\circ$ while speed $> 15\text{ km/h}$.

#### G. Barometric Altitude (BMP280)
- **Hardware**: I2C bus. Calculates atmospheric pressure (hPa) and barometric altitude (meters).

---

### 2.3 Harness Input Sensing (Opto-Isolated Discrete Inputs)

All motorcycle harness signals are debounced and isolated via opto-couplers:
- Left Indicator (`GPIO 19`), Right Indicator (`GPIO 0`), Neutral (`GPIO 35`), High Beam (`GPIO 36`), Horn (`GPIO 37`), Side Stand (`GPIO 41`), Front Brake (`GPIO 42`), Rear Brake (`GPIO 45`), Clutch (`GPIO 46`), Kill Switch (`GPIO 47`), Ignition (`GPIO 48`), Starter (`GPIO 0`).

---

### 2.4 Ride Computer & Analytics Integration

- **Trip A & Trip B**: Distance integration accumulated every 100ms ($\text{dist} = \text{speed} \times \Delta t$).
- **Odometer**: Lifetime vehicle mileage. Automatically saved to NVS every ~10s to preserve flash write endurance.
- **Session Metrics**: Tracks active ride timer (seconds), maximum speed (`maxSpeedKmh`), and session average speed (`avgSpeedKmh`).
- **Fuel Consumption & Range**: Calculates average fuel efficiency ($\text{km/L}$) and remaining range ($\text{km}$).

---

### 2.5 Power Management & Deep Sleep Lifecycle

```
[KEY ON] ---> ACTIVE ---> [KEY OFF] ---> LINGER (5s) ---> SAFE_SHUTDOWN ---> DEEP_SLEEP (<15µA)
                 ^                              |
                 |--- [RE-KEYED WITHIN 5s] -----|
```

- **Permanent Buck Supply**: Board is powered directly from battery positive; ignition key is a logic input on GPIO 48 (`PIN_IN_IGNITION`).
- **Linger Phase**: 5-second delay on key-off to catch stalled engine restarts.
- **Flash Protection**: Flushes dirty NVS settings and closes open SD files before sleep.
- **Deep Sleep Wakeup**:
  ```cpp
  esp_sleep_enable_ext0_wakeup(PIN_IN_IGNITION, HIGH);
  esp_deep_sleep_start();
  ```
  Consumes `< 15 µA` during deep sleep.

---

### 2.6 Safety, Warnings & Buzzer Engine

- **16-Flag Warning Bitmask**: Monitors engine overtemp, oil pressure, low battery, charging fault, low fuel, ABS fault, crash detected, unauthorized movement, GPS lost, SD card fault, etc.
- **Queue Eviction**: Priority queue (`CRITICAL` > `WARNING` > `INFO`). Evicts oldest `INFO` when queue is full.
- **Piezo Buzzer Patterns**:
  - `CRITICAL`: 3 rapid beeps (80ms on / 80ms off).
  - `WARNING`: 1 single beep.
  - `INFO`: Visual alert banner only.
- **Hard Braking Flasher**: Flashes WS2812B NeoPixel ring bright red if vehicle decelerates $> 8\text{ km/h}$ in a single tick.

---

### 2.7 Smart Lighting & Actuators

- **Auto DRL PWM**: 5kHz LEDC PWM on GPIO 47, auto-scaling duty cycle (60–255) based on ambient lux.
- **Hazard Light Relay**: GPIO 48 relay driver for hazard lights.
- **WS2812B NeoPixel Ring**: 24-pixel RGB ring (GPIO 35) for welcome/goodbye animations, theme accents, and emergency brake flashing.

---

### 2.8 GPS Navigation & Time Discipline

- **UART NMEA Parsing**: NEO-6M / NEO-8M via `TinyGPS++` on UART1 (RX 44, TX 43).
- **Telemetry**: Latitude, Longitude, Altitude, Heading, Satellites count.
- **GPS Speed Cross-Check**: Compares GPS speed against wheel hall sensor.
- **RTC Auto-Sync**: Auto-syncs DS3231 RTC and system clock with atomic UTC time.

---

### 2.9 Wireless Connectivity & NimBLE Telemetry

- **NimBLE Stack**: Low-RAM BLE server running on Core 0.
- **GATT Telemetry Service**: Broadcasts JSON telemetry payload at 2 Hz (`6e400002-...`):
  ```json
  {"spd":65,"rpm":5200,"fuel":82,"batt":13.8,"eng_t":88,"odo":14250,"warn":0,"lat":18.5204,"lon":73.8567}
  ```
- **GATT Command API**: Listens on write characteristic (`6e400003-...`) for allow-listed commands (`reset_trip_a`, `reset_trip_b`, `find_bike`).

---

### 2.10 Storage & Mass Logging (NVS / SD)

- **NVS Flash**: Stores settings, odometer, and trip values.
- **SD Card Exporter**: CSV continuous sensor logger + GPX GPS track exporter.

---

## 3. FreeRTOS Scheduling & Core Allocation

| Task Name | Priority | Core | Period / Rate | Function |
| :--- | :---: | :---: | :---: | :--- |
| `Sensor` | 5 | Core 1 (`REALTIME`) | 20 ms (50 Hz) | Hardware ADC, I2C, 1-Wire, Pulse processing |
| `Ride` | 5 | Core 1 (`REALTIME`) | 100 ms (10 Hz) | Distance/time integration & fuel math |
| `Power` | 5 | Core 1 (`REALTIME`) | 100 ms (10 Hz) | Ignition state machine & sleep logic |
| `Lighting` | 5 | Core 1 (`REALTIME`) | 50 ms (20 Hz) | DRL PWM, hazard relay & WS2812B ring |
| `Notif` | 5 | Core 1 (`REALTIME`) | 250 ms (4 Hz) | Warning flags scan & Piezo buzzer chimes |
| `Display` | 4 | Core 1 (`REALTIME`) | 16.6 ms (60 FPS) | LVGL 8.4 UI composition & render loop |
| `Storage` | 2 | Core 0 (`CONNECTIVITY`) | 10 s | NVS settings flush & SD CSV/GPX writes |
| `GPS` | 3 | Core 0 (`CONNECTIVITY`) | 100 ms (10 Hz) | NMEA UART parsing & RTC time discipline |
| `BLE` | 3 | Core 0 (`CONNECTIVITY`) | 500 ms (2 Hz) | NimBLE telemetry notify & command handler |
| `Diag` | 1 | Core 0 (`CONNECTIVITY`) | 2000 ms | Free heap & CPU load diagnostics |
