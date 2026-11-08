# AEZEL — Modular Budget Build Guide

This guide details AEZEL's **modular plug-and-play architecture**, enabling incremental hardware upgrades from a minimum $15 core setup up to a full $55 VCU.

---

## 📑 Table of Contents

- [1. Modular Plug-and-Play Architecture](#1-modular-plug-and-play-architecture)
- [2. Fail-Safe Peripherals Probe](#2-fail-safe-peripherals-probe)
- [3. Staged Hardware Upgrade Roadmap](#3-staged-hardware-upgrade-roadmap)

---

## 1. Modular Plug-and-Play Architecture

AEZEL is designed from the ground up to **never require all components to be installed at once**. You can start riding with a bare-minimum $15 build and add sensors, lighting, and modules whenever your budget permits.

```
[Stage 1: Core Cockpit ($15)] ──> [Stage 2: Lighting & Relays ($25)] ──> [Stage 3: Sensors ($40)] ──> [Stage 4: Full VCU ($55)]
```

---

## 2. Fail-Safe Peripherals Probe

During startup, every subsystem manager (`SensorManager`, `GpsManager`, `StorageManager`) performs a non-blocking probe check on its hardware bus (I2C, SPI, 1-Wire, UART).

- **Missing Sensor Behavior**: If a sensor or module is missing (e.g., no GPS receiver, no IMU, no DS18B20 temperature probe, or no SD card inserted), AEZEL **does NOT crash**.
- **Dashboard Graceful Fallback**: Missing sensors display `"--"` or `0` on the screen and log a low-priority info warning (`GPS_LOST`, `SD_CARD_FAULT`) without disrupting 60 FPS display rendering or ignition control.

---

## 3. Staged Hardware Upgrade Roadmap

### Stage 1: Core Cockpit (Minimum Budget — ~$15 to $20)
* **Components**:
  - ESP32-S3 DevKit Board (~$4)
  - 3.2" ILI9341 TFT Display (~$7)
  - 12V-to-5V Automotive Buck Converter (~$2)
  - Resistors & Rear Wheel Speed Hall Sensor (~$3)
* **Active Features**: 60 FPS Digital Speedometer, Trip Computer, Odometer, 3-Phase OEM Boot Animation, Theme Engine, Ignition Sense, and BLE Telemetry.

---

### Stage 2: Smart Lighting & Actuators (~$10 additional)
* **Components**:
  - 24-Pixel WS2812B NeoPixel RGB Ring (~$3)
  - 12V Automotive Relays (~$4)
  - Piezo Warning Buzzer (~$1)
* **Active Features**: Welcome/Goodbye LED sweeps, Emergency Hard-Braking Red Flasher, Hazard Light Relays, Warning Buzzer Chimes, and Phone Keyless Ignition Relay.

---

### Stage 3: Advanced Sensor Suite (~$15 additional)
* **Components**:
  - MPU6050 6-Axis IMU (~$2)
  - DS18B20 Temperature Probes (~$2)
  - BMP280 Barometer (~$2)
  - SD Card SPI Module (~$2)
* **Active Features**: Dynamic Lean Angle Roll, Pitch, Crash/Fall Detection, Engine Head Temperature, Ambient Temp, Barometric Altitude, and SD CSV/GPX Data Logging.

---

### Stage 4: Full Connected VCU (~$12 additional)
* **Components**:
  - NEO-6M / NEO-8M GPS Receiver (~$8)
  - Electric Seat Lock Solenoid (~$4)
* **Active Features**: GPS Ground Speed Cross-Check, Real-Time Navigation, RTC Atomic Time Sync, and Remote Smartphone Seat Lock Release.
