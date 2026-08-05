# AEZEL — Technical Architecture Comparison: Claude (`main`) vs. Antigravity (`gemini`)

This document provides a detailed side-by-side technical comparison of the **AEZEL Vehicle Control Unit (VCU)** codebase between:
1. **Claude's Base Architecture** (stored on the **`main`** branch)
2. **Antigravity's OEM Enhanced Architecture** (stored on the **`gemini`** branch)

It also documents the synthesis strategy used to merge the best modular patterns from Claude into Antigravity's production feature set.

---

## 📑 Table of Contents

- [1. Executive Subsystem Comparison Matrix](#1-executive-subsystem-comparison-matrix)
- [2. Deep Dive: Key Diagyfferences](#2-deep-dive-key-differences)
  - [2.1 Actuator Safety Interlocks & Remote Control](#21-actuator-safety-interlocks--remote-control)
  - [2.2 Signal Processing & Vehicle Math Library](#22-signal-processing--vehicle-math-library)
  - [2.3 3-Phase OEM Boot Animation & UI Engine](#23-3-phase-oem-boot-animation--ui-engine)
  - [2.4 Turn-by-Turn GPS Navigation Engine](#24-turn-by-turn-gps-navigation-engine)
  - [2.5 Wireless Over-The-Air (OTA) Updates](#25-wireless-over-the-air-ota-updates)
  - [2.6 Automotive CAN Bus / OBD-II Telemetry](#26-automotive-can-bus--obd-ii-telemetry)
  - [2.7 Native Android Companion App & CI/CD](#27-native-android-companion-app--cicd)
- [3. Synthesized Architecture on the `gemini` Branch](#3-synthesized-architecture-on-the-gemini-branch)

---

## 1. Executive Subsystem Comparison Matrix

| Subsystem / Feature | Claude's Version (`main` branch) | Antigravity's Version (`gemini` branch) | Merged / Synthesis Decision |
| :--- | :--- | :--- | :--- |
| **Actuator Safety Interlocks** | Dedicated [`RemoteControlManager`](file:///workspaces/AEZEL/src/managers/RemoteControlManager.h) single point of control for horn, hazards, signals, lock. Returns `cmd_result` status to phone app. | Direct command dispatcher inside `BleManager` with `NotificationManager` warning notifications. | **MERGED INTO `gemini`**: Integrated `RemoteControlManager` while preserving Antigravity's **Neutral Safety Interlock for Remote Engine Start**. |
| **Vehicle Math Library** | Decoupled [`VehicleMath.h`](file:///workspaces/AEZEL/include/VehicleMath.h) & [`VehicleMath.cpp`](file:///workspaces/AEZEL/src/VehicleMath.cpp) with native PlatformIO unit testing ([`test_vehicle_math.cpp`](file:///workspaces/AEZEL/test/native/test_vehicle_math.cpp)). | Inline Exponential Moving Average (EMA) filters inside `SensorManager.cpp`. | **MERGED INTO `gemini`**: Integrated `VehicleMath` so math formulas can be unit-tested without hardware dependencies. |
| **3-Phase OEM Boot Animation** | Basic welcome pulse and segment test in `DisplayManager.cpp`. | **Advanced 3-Phase OEM Boot Animation**: Splash Logo reveal → 270° Tachometer Arc Sweep + `"188"` segment test + Bulb test → 60 FPS live telemetry. | **ANTIGRAVITY SUPERIOR**: Preserved on `gemini`. |
| **Turn-by-Turn Navigation Engine** | Basic roadmap placeholder in `docs/roadmap.md`. | **Full Turn-by-Turn Navigation Engine**: BLE `nav_update` payload parsing for turn arrows (⬆ ◀ ▶ ↩), street name, distance, ETA, and dashboard UI. | **ANTIGRAVITY SUPERIOR**: Preserved on `gemini`. |
| **Wireless OTA Wi-Fi Updates** | Not implemented. | **[`OtaManager`](file:///workspaces/AEZEL/src/managers/OtaManager.h)**: Serves embedded WebServer at `http://192.168.4.1` for wireless `.bin` firmware uploads. | **ANTIGRAVITY SUPERIOR**: Preserved on `gemini`. |
| **Automotive CAN Bus / OBD-II** | Not implemented. | **[`CanManager`](file:///workspaces/AEZEL/src/managers/CanManager.h)**: 500 kbps TWAI controller polling ECU PIDs (RPM, Speed, Coolant Temp, Throttle). | **ANTIGRAVITY SUPERIOR**: Preserved on `gemini`. |
| **Remote Engine Start** | Basic `remoteStart()` stub without neutral interlock check. | **Sequential Remote Start**: Checks Neutral (`gear == N`) and Kill Switch, energizes ignition relay, primes fuel pump, and pulses starter solenoid for 1.2s or until `RPM > 800`. | **ANTIGRAVITY SUPERIOR**: Preserved on `gemini`. |
| **Native Android Companion App** | Written in markdown doc. | **Complete Standalone App on `android-app` branch**: Native Jetpack Compose, Coroutines BLE stack, 4 screens, GitHub Actions CI/CD. | **ANTIGRAVITY SUPERIOR**: Maintained on `android-app`. |

---

## 2. Deep Dive: Key Differences

### 2.1 Actuator Safety Interlocks & Remote Control
- **Claude (`main`)**: Routes all incoming BLE commands through [`RemoteControlManager`](file:///workspaces/AEZEL/src/managers/RemoteControlManager.h), which manages state transition rules for relays (horn pulse, hazard toggle, turn indicators, horn off). Outputs `cmd_result` telemetry string.
- **Antigravity (`gemini`)**: Implemented strict safety interlocks for **Remote Engine Start**, verifying that the vehicle transmission is in **Neutral (`gear == N`)** and **Kill Switch is OFF** before engaging `PIN_OUT_STARTER_RELAY`.

### 2.2 Signal Processing & Vehicle Math Library
- **Claude (`main`)**: Introduced [`VehicleMath.h`](file:///workspaces/AEZEL/include/VehicleMath.h) to decouple raw ADC integer scaling, fuel tank sender resistance math, and Exponential Moving Average (EMA) smoothing from hardware GPIO pin calls.
- **Antigravity (`gemini`)**: Embedded signal processing within `SensorManager.cpp`.

### 2.3 3-Phase OEM Boot Animation & UI Engine
- **Claude (`main`)**: Uses standard LVGL 8.4 dashboard widgets with basic initialization.
- **Antigravity (`gemini`)**: Features an authentic OEM 3-phase startup sequence:
  1. *Phase 1 (0–500ms)*: Splash Screen logo (`AEZEL VCU SYSTEM CHECK...`) + progress bar fill + NeoPixel blue welcome chase.
  2. *Phase 2 (500–1200ms)*: Dashboard fade-in + Tachometer arc sweep (`0 → 12,000 → 0 RPM`) + Speedometer segment test (`"188"`) + Indicator bulb test.
  3. *Phase 3 (1200ms+)*: Handover to 60 FPS live telemetry rendering.

### 2.4 Turn-by-Turn GPS Navigation Engine
- **Claude (`main`)**: Mentioned in `docs/roadmap.md`.
- **Antigravity (`gemini`)**: Fully implemented in `DisplayManager.cpp` and `BleManager.cpp`. Accepts live companion app navigation payloads (`nav_update`) and renders turn arrows (⬆ ◀ ▶ ↩), remaining distance, street names, and ETA countdowns.

### 2.5 Wireless Over-The-Air (OTA) Updates
- **Claude (`main`)**: Requires USB serial cable connection for flashing.
- **Antigravity (`gemini`)**: [`OtaManager`](file:///workspaces/AEZEL/src/managers/OtaManager.h) launches an embedded Access Point (`"AEZEL-VCU-AP"`) and Web Server (`http://192.168.4.1`) allowing wireless `.bin` firmware updates.

### 2.6 Automotive CAN Bus / OBD-II Telemetry
- **Claude (`main`)**: Hardware GPIO sensor reading only.
- **Antigravity (`gemini`)**: [`CanManager`](file:///workspaces/AEZEL/src/managers/CanManager.h) utilizes the ESP32-S3's hardware TWAI controller on `GPIO 4` (TX) & `GPIO 5` (RX) at 500 kbps for OBD-II vehicle polling.

---

## 3. Synthesized Architecture on the `gemini` Branch

The **`gemini`** branch now combines the **modular software architecture of Claude** with the **advanced OEM feature set of Antigravity**:

1. **`RemoteControlManager`**: Centralized actuator state machine with safety status feedback.
2. **`VehicleMath`**: Decoupled, unit-tested math library for signal filtering and scaling.
3. **`DisplayManager`**: 3-Phase OEM Boot Animation + 60 FPS LVGL Instrumentation + Turn-by-Turn Navigation UI.
4. **`OtaManager` & `CanManager`**: Wireless Wi-Fi firmware flashing + 500kbps TWAI CAN bus controller.
5. **Remote Engine Start**: Enforced Neutral safety interlocks.
