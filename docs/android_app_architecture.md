# AEZEL Android Companion App Specification & CI/CD Pipeline

This document details the native Android Companion Application for the **AEZEL Smart Motorcycle Cockpit & VCU**, built on the **`android-app`** branch.

---

## 📑 Table of Contents

- [1. Architecture Overview](#1-architecture-overview)
- [2. Jetpack Compose UI & Screen Structure](#2-jetpack-compose-ui--screen-structure)
- [3. BLE GATT Communication Subsystem](#3-ble-gatt-communication-subsystem)
- [4. Remote Engine Start & Safety Interlocks](#4-remote-engine-start--safety-interlocks)
- [5. Turn-by-Turn Navigation Push Engine](#5-turn-by-turn-navigation-push-engine)
- [6. GitHub Actions CI/CD Automated Build Pipeline](#6-github-actions-cicd-automated-build-pipeline)

---

## 1. Architecture Overview

The AEZEL Android App is built using **Kotlin**, **Jetpack Compose (Material 3)**, and **Kotlin Coroutines**. It connects wirelessly over Bluetooth Low Energy (BLE) to the AEZEL ESP32-S3 VCU.

```
+-------------------------------------------------------------------+
|                     AEZEL ANDROID COMPANION APP                   |
+-------------------------------------------------------------------+
|  [DashboardScreen]    [RemoteControlScreen]   [NavigationScreen] |
|   - Speedometer Arc    - 🚀 Remote Start        - Street Name     |
|   - RPM Arc            - Keyless Ignition       - Turn Icon ⬆◀▶  |
|   - Battery/Fuel       - Seat Lock Solenoid     - Push to Dash    |
+-------------------------------------------------------------------+
                                 │
                     Kotlin StateFlow Stream
                                 ▼
+-------------------------------------------------------------------+
|            AezelBleManager (Coroutines BLE GATT Stack)            |
|  - Auto-Discovery ("Phoenix Cockpit" / "AEZEL VCU")               |
|  - Subscribes to Telemetry Characteristic (6e400002-...)          |
|  - Writes JSON Commands to Actuator Characteristic (6e400003-...) |
+-------------------------------------------------------------------+
                                 │ 2 Hz BLE Telemetry / Write Commands
                                 ▼
+-------------------------------------------------------------------+
|                    AEZEL ESP32-S3 VCU HARDWARE                    |
+-------------------------------------------------------------------+
```

---

## 2. Jetpack Compose UI & Screen Structure

The UI follows AEZEL's **Cyberpunk Dark Aesthetic** (`#07090E` background, `#00D4FF` Cyan accent, `#FF1744` Red alert, `#39FF14` Lime neon):

1. **DashboardScreen**:
   * Glowing animated 60 FPS Speedometer circular gauge.
   * RPM Tachometer arc.
   * Central Gear position badge (`N`, `1`–`5`, `A`).
   * Battery Voltage (`12.6V`) and Fuel Level progress bar (`%`).
   * Engine Head & Ambient Temperature chips.
   * Active Warning Alert Badges (Low Fuel, Engine Overheat, Theft Alarm, Side Stand Down).

2. **RemoteControlScreen**:
   * Prominent **🚀 REMOTE START ENGINE** button with Neutral Safety Interlock confirmation popup.
   * **⚡ Keyless Ignition** toggle switch.
   * **📣 Horn Pulse** relay actuator.
   * **🚨 Hazard Flashers** toggle.
   * **🔓 Seat Lock Release** solenoid trigger.
   * **📍 Find My Bike** panic alarm.

3. **NavigationScreen**:
   * Target street name input field.
   * Turn direction selector (Straight ⬆, Left ◀, Right ▶, U-Turn ↩).
   * Distance slider ($10\text{m} \rightarrow 2000\text{m}$).
   * ETA minutes slider ($1 \rightarrow 60\text{ Mins}$).
   * **🗺️ Push to Motorcycle Cockpit** button.

4. **AnalyticsScreen**:
   * Peak Speed, Average Speed, Ride Duration, Odometer, and GPS coordinate pin.
   * GPX / CSV log exporter.

---

## 3. BLE GATT Communication Subsystem

* **Target Device Name**: `"Phoenix Cockpit"` or `"AEZEL VCU"`
* **Service UUID**: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
* **Telemetry Notification UUID**: `6e400002-b5a3-f393-e0a9-e50e24dcca9e` (Parses 2 Hz JSON payload: `spd`, `rpm`, `fuel`, `batt`, `eng_t`, `odo`, `warn`, `lat`, `lon`).
* **Command Write UUID**: `6e400003-b5a3-f393-e0a9-e50e24dcca9e` (Writes JSON command strings).

---

## 4. Remote Engine Start & Safety Interlocks

When the user taps **🚀 REMOTE START ENGINE**:
1. `AezelBleManager` checks the live telemetry gear state.
2. If `gear != "N"`, an **AlertDialog** displays: `"SAFETY INTERLOCK VIOLATION: Vehicle is not in Neutral (Current Gear: 2). Shift to Neutral (N) before remote starting."`
3. When in Neutral (`N`), `AezelBleManager` sends `{"cmd":"remote_start_engine"}`.
4. The VCU energizes the ignition relay, primes the fuel pump, and fires a 1.2s pulse to the starter motor solenoid relay.

---

## 5. Turn-by-Turn Navigation Push Engine

When the rider inputs a navigation target on the app, tapping **🗺️ PUSH TO MOTORCYCLE DASHBOARD** sends the JSON payload:

```json
{
  "cmd": "nav_update",
  "dist": 150,
  "turn": 1,
  "street": "MG Road",
  "eta": 12
}
```

The VCU LVGL dashboard immediately displays the turn arrow symbol, remaining distance, street name, and ETA.

---

## 6. GitHub Actions CI/CD Automated Build Pipeline

The workflow file [`.github/workflows/android_build.yml`](file:///workspaces/AEZEL/.github/workflows/android_build.yml) automatically builds the Android app on GitHub servers:

* **Trigger**: Every `push` or `pull_request` to `android-app` or `main`.
* **Runner**: `ubuntu-latest` with JDK 17 (Zulu).
* **Build Step**: `./gradlew assembleDebug --stacktrace`.
* **Artifact Output**: The compiled APK (`app-debug.apk`) is automatically uploaded as a downloadable GitHub Release Artifact on every commit!
