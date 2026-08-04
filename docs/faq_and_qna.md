# AEZEL — Frequently Asked Questions & Technical Q&A Reference

This document compiles technical questions, answers, architectural decisions, and troubleshooting references for the **AEZEL** Smart Motorcycle Cockpit and Vehicle Control Unit (VCU) platform.

---

## 📑 Table of Contents

- [1. General Architecture & Design Decisions](#1-general-architecture--design-decisions)
- [2. Power Management & Electrical System](#2-power-management--electrical-system)
- [3. Animations & UI Engine](#3-animations--ui-engine)
- [4. GPS Tracking & Security](#4-gps-tracking--security)
- [5. Open Source Licensing & Repository Structure](#5-open-source-licensing--repository-structure)
- [6. Build System & Simulation Tooling](#6-build-system--simulation-tooling)

---

## 1. General Architecture & Design Decisions

### Q1: Why is AEZEL described as a Vehicle Control Unit (VCU) rather than just a digital dashboard?
**A:** A digital dashboard only displays data. AEZEL acts as a full **Vehicle Control Unit (VCU)** because it directly processes discrete harness inputs (indicators, brake switches, clutch, side stand, kill switch, ignition), manages power state transitions, executes safety heuristics (crash detection, overtemperature shutdown, battery fault alarms), controls smart lighting/actuators (auto DRL PWM, hazard relay, NeoPixel accent lighting), logs telemetry to SD storage, and exposes a GATT telemetry server over Bluetooth.

### Q2: Why use a single mutex-guarded `SharedState` instead of global variables?
**A:** In multi-tasking FreeRTOS applications, allowing multiple tasks to read and write un-guarded global variables leads to race conditions, partial memory reads, and crash bugs. `SharedState` provides thread-safe `snapshot()` and `update()` accessors guarded by a FreeRTOS mutex, ensuring atomic state reads across task boundaries.

### Q3: Why split FreeRTOS tasks across two cores (`CORE_REALTIME` vs `CORE_CONNECTIVITY`)?
**A:** 
- **Core 1 (`CORE_REALTIME`)**: Dedicated to latency-critical tasks: 60 FPS LVGL display rendering, 50 Hz sensor sampling, safety checks, and lighting PWM.
- **Core 0 (`CORE_CONNECTIVITY`)**: Handles tasks that can stall for milliseconds on I/O: SD card mass storage writes, BLE wireless transmission, GPS UART parsing, and diagnostics.
This ensures a slow SD card write or a flaky BLE connection can never drop a display frame or delay a crash alert.

---

## 2. Power Management & Electrical System

### Q4: How does AEZEL know when the ignition key is turned off?
**A:** The ignition switch line from the motorcycle harness connects to **GPIO 48 (`PIN_IN_IGNITION`)** via an opto-isolator. When the key is turned ON, GPIO 48 reads `3.3V` (`HIGH`). When turned OFF, it drops to `0V` (`LOW`).

### Q5: Won't turning off the key cut power to the ESP32 before it can display the shutdown animation?
**A:** No. In OEM automotive design, the VCU is permanently connected to **Battery Positive (+12V)** through a fused automotive buck converter. Turning off the key toggles a logic signal on GPIO 48, but does **NOT** disconnect the battery from the buck converter.

### Q6: How does the shutdown sequence protect flash memory from corruption?
**A:** When key-off is detected, AEZEL enters a 5-second `LINGER` phase. If ignition remains OFF, `PowerManager` executes `SAFE_SHUTDOWN`:
1. Plays the WS2812B NeoPixel fade-out animation (`playGoodbyeAnimation`).
2. Renders the LVGL `"See you next ride"` screen (`showGoodbyeScreen`).
3. Flushes dirty NVS settings (odometer, trip values).
4. Safely closes open SD card GPX/CSV log files.
5. Invokes `esp_deep_sleep_start()`.

### Q7: Will AEZEL drain my motorcycle battery in deep sleep?
**A:** No. In **Deep Sleep**, the ESP32-S3 draws less than **10–15 µA** (microamps)—meaning it would take years to discharge a standard motorcycle battery. When the key is turned back ON, GPIO 48 (`PIN_IN_IGNITION`) triggers an instant `ext0` hardware wake reset.

---

## 3. Animations & UI Engine

### Q8: How does the 3-phase OEM boot animation work?
**A:** On power-on, [`DisplayManager`](file:///workspaces/AEZEL/src/managers/DisplayManager.cpp) runs a 3-phase self-test sequence:
1. **Phase 1 (0–500 ms)**: Renders `AEZEL VCU SYSTEM CHECK...` splash screen with progress bar + NeoPixel welcome pulse.
2. **Phase 2 (500–1200 ms)**: Transitions to Main Dashboard. Tachometer arc gauge smoothly sweeps `0 → 12,000 RPM → 0`, Speedometer displays `"188"` segment test, and all status indicators (Left/Right Turn, Neutral, High Beam) light up for bulb verification.
3. **Phase 3 (1200 ms+)**: Self-test ends, indicators return to live hardware states, and 60 FPS live telemetry begins.

### Q9: How can I preview the boot and shutdown animations without flashing hardware?
**A:** Open the interactive web simulation preview located at [`docs/preview/index.html`](file:///workspaces/AEZEL/docs/preview/index.html). Toggling the Ignition Switch button simulates the key ON/OFF sequence, welcome pulse, 3-phase boot animation, throttle speed/RPM arc sweep, theme changes, and goodbye shutdown sequence.

---

## 4. GPS Tracking & Security

### Q10: Does AEZEL have 24/7 continuous GPS tracking while parked?
**A:** No. AEZEL tracks GPS position **only when the ignition key is ON** (and during the 5-second linger phase). When ignition is turned OFF, the system enters deep sleep (<15 µA draw) and powers down peripherals to protect the vehicle battery.

### Q11: What would be required to enable 24/7 remote tracking in future updates?
**A:** 24/7 tracking while parked requires a cellular modem (e.g. SIM7600 4G/LTE/NB-IoT) to transmit coordinates over cellular networks without relying on Bluetooth proximity, along with periodic RTC timer wake-ups during deep sleep. This is planned in [`docs/roadmap.md`](file:///workspaces/AEZEL/docs/roadmap.md) (Phase 2).

---

## 5. Open Source Licensing & Repository Structure

### Q12: Why was Apache License 2.0 chosen for AEZEL?
**A:** Apache 2.0 allows commercial use, modification, distribution, and private use while preserving copyright attribution. Crucially for embedded hardware platforms, it includes an **explicit patent grant** and legal protection for contributors.

### Q13: Do I need to register or apply anywhere for an open-source license?
**A:** No application or cost is required. Including a valid `LICENSE` text file in the repository root legally establishes the open-source terms.

---

## 6. Build System & Simulation Tooling

### Q14: How does the one-command `./setup.sh` script work?
**A:** Executing [`./setup.sh`](file:///workspaces/AEZEL/setup.sh) automatically checks Python 3, installs/verifies PlatformIO CLI (`pio`) and Wokwi CLI (`wokwi-cli`), compiles the AEZEL firmware (`esp32-phoenix` environment), and generates the binary artifact [`.pio/build/esp32-phoenix/firmware.bin`](file:///workspaces/AEZEL/.pio/build/esp32-phoenix/firmware.bin).

### Q15: Why are `diagram.json` and `wokwi.toml` exclusive to the `Wokwi-Simulation` branch?
**A:** The `main` branch is kept clean as the core production codebase. Simulation testbench files (`diagram.json` and `wokwi.toml`) are isolated on the `Wokwi-Simulation` branch so developers testing virtual hardware can work in a dedicated simulation workspace without cluttering the main branch.
