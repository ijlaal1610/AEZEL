# AEZEL — Subsystem Managers Developer Guide

This document provides a low-level architectural guide for all 9 modular subsystem managers in `src/managers/`.

---

## 📑 Table of Contents

- [1. Subsystem Manager Architecture](#1-subsystem-manager-architecture)
- [2. Detailed Manager Breakdown](#2-detailed-manager-breakdown)
  - [2.1 SensorManager](#21-sensormanager)
  - [2.2 RideManager](#22-ridemanager)
  - [2.3 DisplayManager](#23-displaymanager)
  - [2.4 PowerManager](#24-powermanager)
  - [2.5 NotificationManager](#25-notificationmanager)
  - [2.6 LightingManager](#26-lightingmanager)
  - [2.7 GpsManager](#27-gpsmanager)
  - [2.8 BleManager](#28-blemanager)
  - [2.9 StorageManager](#29-storagemanager)

---

## 1. Subsystem Manager Architecture

Every manager in AEZEL follows a standardized, testable C++ Singleton pattern:

```cpp
class SubsystemManager {
public:
    static SubsystemManager& instance() { static SubsystemManager m; return m; }
    void begin();
    static void taskEntry(void* pv);
    void tick();
private:
    SubsystemManager() = default;
};
```

### Guiding Principles:
1. **Decoupled Memory**: No manager holds pointers to another manager. Communication happens exclusively through [`SharedState`](file:///workspaces/AEZEL/include/DataModel.h).
2. **Dedicated FreeRTOS Tasks**: Each manager runs inside its own FreeRTOS task with an assigned core, priority, and tick period.
3. **Fail-Safe Execution**: If a sensor fails or an I2C device disconnects, the manager logs the fault to `activeWarnings` without crashing the render loop.

---

## 2. Detailed Manager Breakdown

### 2.1 SensorManager
- **Files**: [`SensorManager.h`](file:///workspaces/AEZEL/src/managers/SensorManager.h), [`SensorManager.cpp`](file:///workspaces/AEZEL/src/managers/SensorManager.cpp)
- **Task**: `Sensor` (Priority 5, Core 1 `REALTIME`, Rate: 50 Hz / 20ms)
- **Responsibilities**:
  - Attached to GPIO 18 (Speed Hall) & GPIO 17 (RPM Pickup) hardware interrupts.
  - Samples analog ADCs for Fuel Sender (GPIO 1), Battery Voltage (GPIO 2), Charging Voltage (GPIO 3), and Ambient Light (GPIO 16).
  - Queries DS18B20 1-Wire temperature sensors on GPIO 15.
  - Performs I2C reads on MPU6050 (IMU lean angle, pitch, acceleration) and BMP280 (barometric pressure, altitude).
  - Debounces all 12 discrete harness inputs (indicators, neutral, high beam, brakes, clutch, side stand, kill switch, ignition).
  - Writes raw sensor data into `SharedState`.

---

### 2.2 RideManager
- **Files**: [`RideManager.h`](file:///workspaces/AEZEL/src/managers/RideManager.h), [`RideManager.cpp`](file:///workspaces/AEZEL/src/managers/RideManager.cpp)
- **Task**: `Ride` (Priority 5, Core 1 `REALTIME`, Rate: 10 Hz / 100ms)
- **Responsibilities**:
  - Integrates speed over time ($\Delta d = v \cdot \Delta t$) to update Trip A, Trip B, and total Odometer.
  - Tracks ride timer (seconds elapsed since ignition ON).
  - Calculates session peak speed (`maxSpeedKmh`) and session rolling average speed (`avgSpeedKmh`).
  - Computes average fuel consumption ($\text{km/L}$) and estimates remaining fuel range ($\text{km}$).
  - Exposes `resetTripA()` and `resetTripB()` public methods.

---

### 2.3 DisplayManager
- **Files**: [`DisplayManager.h`](file:///workspaces/AEZEL/src/managers/DisplayManager.h), [`DisplayManager.cpp`](file:///workspaces/AEZEL/src/managers/DisplayManager.cpp)
- **Task**: `Display` (Priority 4, Core 1 `REALTIME`, Rate: 60 FPS / 16.6ms)
- **Responsibilities**:
  - Initializes TFT ILI9341 display and LVGL 8.4 graphics engine.
  - Executes 3-phase OEM boot animation (Splash Screen logo -> 270° arc tachometer sweep + 188 segment test + indicator bulb check -> live dashboard).
  - Renders 60 FPS speedometer, sweep tachometer, gear position, fuel progress bar, temperature gauges, and top status icons.
  - Manages screen navigation flow (`MAIN_DASHBOARD`, `TRIP_INFO`, `SETTINGS`).
  - Swaps UI theme color tokens (`MODERN_DIGITAL`, `SPORT`, `NEON`, `RETRO`).
  - Displays centered `"See you next ride"` goodbye screen on shutdown.

---

### 2.4 PowerManager
- **Files**: [`PowerManager.h`](file:///workspaces/AEZEL/src/managers/PowerManager.h), [`PowerManager.cpp`](file:///workspaces/AEZEL/src/managers/PowerManager.cpp)
- **Task**: `Power` (Priority 5, Core 1 `REALTIME`, Rate: 10 Hz / 100ms)
- **Responsibilities**:
  - Controls power state machine (`ACTIVE` → `LINGER` → `SAFE_SHUTDOWN` → `DEEP_SLEEP`).
  - Monitors ignition logic signal on GPIO 48 (`PIN_IN_IGNITION`).
  - Implements 5-second `LINGER` window on key-off to catch stalled engine restarts.
  - Triggers pre-sleep storage flush (NVS odometer & SD files).
  - Configures `ext0` wake-up on GPIO 48 (`HIGH`) and enters ESP32 deep sleep (`< 15 µA` current draw).

---

### 2.5 NotificationManager
- **Files**: [`NotificationManager.h`](file:///workspaces/AEZEL/src/managers/NotificationManager.h), [`NotificationManager.cpp`](file:///workspaces/AEZEL/src/managers/NotificationManager.cpp)
- **Task**: `Notif` (Priority 5, Core 1 `REALTIME`, Rate: 4 Hz / 250ms)
- **Responsibilities**:
  - Monitors 16-flag warning bitmask (`activeWarnings`) in `SharedState`.
  - Maintains prioritized notification queue (`CRITICAL` > `WARNING` > `INFO`).
  - Drops oldest `INFO` alert when queue is full.
  - Drives Piezo warning buzzer (GPIO 36) with patterned chimes (3 rapid beeps for critical alerts).
  - Renders top alert banners on the LVGL dashboard.

---

### 2.6 LightingManager
- **Files**: [`LightingManager.h`](file:///workspaces/AEZEL/src/managers/LightingManager.h), [`LightingManager.cpp`](file:///workspaces/AEZEL/src/managers/LightingManager.cpp)
- **Task**: `Lighting` (Priority 5, Core 1 `REALTIME`, Rate: 20 Hz / 50ms)
- **Responsibilities**:
  - Controls Daytime Running Light (DRL) PWM on GPIO 47 via LEDC timer (auto-brightness scaling).
  - Drives Hazard light relay on GPIO 48.
  - Controls 24-pixel WS2812B NeoPixel RGB accent ring on GPIO 35.
  - Executes welcome pulse and goodbye fade animations.
  - Detects hard braking ($>8\text{ km/h}$ drop per tick) and flashes NeoPixel ring red.

---

### 2.7 GpsManager
- **Files**: [`GpsManager.h`](file:///workspaces/AEZEL/src/managers/GpsManager.h), [`GpsManager.cpp`](file:///workspaces/AEZEL/src/managers/GpsManager.cpp)
- **Task**: `GPS` (Priority 3, Core 0 `CONNECTIVITY`, Rate: 10 Hz / 100ms)
- **Responsibilities**:
  - Parses NMEA sentences from NEO-6M/8M GPS receiver on UART1 (RX 44, TX 43).
  - Updates latitude, longitude, altitude, heading, and satellite count in `SharedState`.
  - Cross-checks GPS speed against wheel hall sensor.
  - Auto-synchronizes ESP32 system clock and DS3231 RTC module with UTC time.

---

### 2.8 BleManager
- **Files**: [`BleManager.h`](file:///workspaces/AEZEL/src/managers/BleManager.h), [`BleManager.cpp`](file:///workspaces/AEZEL/src/managers/BleManager.cpp)
- **Task**: `BLE` (Priority 3, Core 0 `CONNECTIVITY`, Rate: 2 Hz / 500ms)
- **Responsibilities**:
  - Initializes NimBLE 1.4 GATT server (`Phoenix Cockpit`).
  - Broadcasts 2 Hz JSON telemetry notifications (`spd`, `rpm`, `fuel`, `batt`, `eng_t`, `odo`, `warn`, `lat`, `lon`).
  - Enables MITM protection and Passkey bonding.
  - Receives and dispatches GATT write commands (`reset_trip_a`, `reset_trip_b`, `find_bike`).

---

### 2.9 StorageManager
- **Files**: [`StorageManager.h`](file:///workspaces/AEZEL/src/managers/StorageManager.h), [`StorageManager.cpp`](file:///workspaces/AEZEL/src/managers/StorageManager.cpp)
- **Task**: `Storage` (Priority 2, Core 0 `CONNECTIVITY`, Rate: 0.1 Hz / 10s)
- **Responsibilities**:
  - Manages Non-Volatile Storage (NVS) flash key-value pairs for odometer, trips, and user settings.
  - Initializes SD card mass storage over SPI.
  - Formats and flushes continuous CSV sensor logs.
  - Generates standard GPX 1.1 XML ride track files for export.
