# AEZEL — Modular Budget Build & Smartphone Control Guide

This guide details AEZEL's **modular plug-and-play architecture** (enabling incremental budget-friendly hardware upgrades) and the **full smartphone remote control system**.

---

## 📑 Table of Contents

- [1. Modular Plug-and-Play Architecture](#1-modular-plug-and-play-architecture)
- [2. Staged Upgrade Roadmap (Minimum to Full Build)](#2-staged-upgrade-roadmap-minimum-to-full-build)
- [3. Smartphone Remote Control System (BLE GATT API)](#3-smartphone-remote-control-system-ble-gatt-api)
- [4. Remote Command Reference](#4-remote-command-reference)

---

## 1. Modular Plug-and-Play Architecture

AEZEL is designed from the ground up to **never require all components to be installed at once**. 

### Fail-Safe Peripherals Probe
During boot, every subsystem manager (`SensorManager`, `GpsManager`, `StorageManager`) performs a non-blocking probe check on its hardware bus (I2C, SPI, 1-Wire, UART).
- **Missing Sensor Behavior**: If a sensor or module is missing (e.g. no GPS module, no IMU, no DS18B20 temperature probe, no SD card inserted), AEZEL **does NOT crash**.
- **Dashboard Graceful Fallback**: Missing sensors display `"--"` or `0` on the screen and log a low-priority warning flag without disrupting real-time display rendering or ignition control.

---

## 2. Staged Upgrade Roadmap (Minimum to Full Build)

You can build AEZEL incrementally according to your budget:

```
[Stage 1: Core Cockpit ($15)] ──> [Stage 2: Lighting & Relays ($25)] ──> [Stage 3: Sensors ($40)] ──> [Stage 4: Full VCU ($55)]
```

### Stage 1: Core Cockpit (Minimum Budget — ~$15–$20)
* **Components**: ESP32-S3 DevKit ($4) + ILI9341 3.2" TFT Screen ($7) + 12V-to-5V Buck Converter ($2) + Resistors & Speed Hall Sensor ($3).
* **Active Features**: 60 FPS Digital Speedometer, Trip Computer, Odometer, 3-Phase OEM Boot Animation, Theme Engine, Ignition Sensing, and BLE Telemetry.

### Stage 2: Smart Lighting & Actuators (~$10 additional)
* **Components**: 24-Pixel WS2812B NeoPixel Ring ($3) + 12V Relays ($4) + Piezo Buzzer ($1).
* **Active Features**: Welcome/Goodbye LED sweeps, Emergency Hard-Braking Red Flasher, Hazard Light Relays, Warning Buzzer Chimes, and Remote Keyless Ignition Relay.

### Stage 3: Advanced Sensor Suite (~$15 additional)
* **Components**: MPU6050 IMU ($2) + DS18B20 Temp Sensors ($2) + BMP280 Barometer ($2) + SD Card Module ($2).
* **Active Features**: Dynamic Lean Angle Roll, Pitch, Crash/Fall Detection, Engine Head Temperature, Ambient Temp, Barometric Altitude, and SD CSV/GPX Data Logging.

### Stage 4: Full Connected VCU (~$12 additional)
* **Components**: NEO-6M / NEO-8M GPS Module ($8) + Electric Seat Lock Solenoid ($4).
* **Active Features**: GPS Ground Speed Cross-Check, Real-Time Navigation, RTC Atomic Time Sync, and Remote Smartphone Seat Lock Release.

---

## 3. Smartphone Remote Control System (BLE GATT API)

AEZEL exposes a secure **BLE GATT Write API** on characteristic `6e400003-b5a3-f393-e0a9-e50e24dcca9e` allowing full smartphone control:

```
+------------------------+                        +----------------------------------+
|  Smartphone Remote App |  ─── BLE GATT Write ──> | AEZEL VCU (Core 0 -> Core 1)     |
|  (Android / iOS)       |                        | - Keyless Ignition Relay         |
+------------------------+                        | - Horn Beep & Panic Alarm        |
                                                  | - Hazard & Indicator Relays      |
                                                  | - Seat / Trunk Lock Solenoid    |
                                                  +----------------------------------+
```

---

## 4. Remote Command Reference

Commands are sent as JSON packets over BLE:

| Action | JSON Command Payload | Description / Hardware Response |
| :--- | :--- | :--- |
| **⚡ Keyless Ignition** | `{"cmd":"remote_ignition_toggle"}` | Toggles VCU ignition relay (Keyless start/stop) |
| **⚡ Force Ignition ON** | `{"cmd":"remote_ignition_on"}` | Energizes VCU ignition circuit via smartphone |
| **🔑 Force Ignition OFF** | `{"cmd":"remote_ignition_off"}` | Disengages ignition circuit & starts shutdown |
| **📣 Horn Beep** | `{"cmd":"remote_horn_beep"}` | Fires 300ms pulse on 12V Horn relay |
| **🚨 Hazard Flasher** | `{"cmd":"remote_hazard_toggle"}` | Toggles dual hazard flasher relays |
| **◀ Turn Left** | `{"cmd":"remote_indicator_left"}` | Activates left turn signal output |
| **▶ Turn Right** | `{"cmd":"remote_indicator_right"}` | Activates right turn signal output |
| **🔓 Seat / Trunk Release** | `{"cmd":"remote_seat_release"}` | Fires 500ms impulse pulse to electric seat solenoid |
| **📍 Find My Bike** | `{"cmd":"find_bike"}` | Flashes NeoPixel ring red & sounds piezo buzzer alarm |
| **🎨 Change Theme** | `{"cmd":"set_theme","val":"sport"}` | Swaps live dashboard color scheme |
| **🔄 Reset Trip A** | `{"cmd":"reset_trip_a"}` | Clears active Trip A distance counter |
