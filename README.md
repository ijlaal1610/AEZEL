# 📱 AEZEL — Native Android Companion Application

[![Build Android APK](https://github.com/ijlaal1610/AEZEL/actions/workflows/android_build.yml/badge.svg?branch=android-app)](https://github.com/ijlaal1610/AEZEL/actions/workflows/android_build.yml)

Welcome to the dedicated **`android-app`** branch of **AEZEL**. This branch contains the standalone native Android companion application built with **Kotlin**, **Jetpack Compose (Material 3)**, **Coroutines**, and **Bluetooth Low Energy (BLE GATT)**.

---

## 📱 App Highlights & Architecture

* **🎨 Cyberpunk Dark UI**: Jetpack Compose Material 3 dark aesthetic matching the AEZEL VCU dashboard (`#07090E` background, `#00D4FF` Cyan accent).
* **⚡ BLE GATT Manager**: Coroutine-driven Bluetooth stack auto-connecting to `"Phoenix Cockpit"` / `"AEZEL VCU"` (`6e400001-...`).
* **📊 60 FPS Animated Cockpit**: Speedometer arc, RPM tachometer, Gear position (`N`, `1`–`5`, `A`), battery voltage, fuel percentage, engine/ambient temperatures, and warning alert badges.
* **🚀 Remote Engine Start & Remote Control**: Keyless ignition, **Remote Engine Start** (with Neutral safety interlock verification popup), Horn pulse, Hazard flasher, Turn indicators, Electric seat lock release solenoid, and Find My Bike panic alarm.
* **🗺️ Turn-by-Turn Navigation Engine**: Input street names, turn arrows (⬆ ◀ ▶ ↩), distance, and ETA to push directly to the motorcycle cockpit screen.
* **📈 Ride Analytics**: Peak speed, average speed, ride timer, GPS pin coordinates (`latitude`, `longitude`), and GPX/CSV exporter.

---

## 🏗️ Repository Layout (`android-app` Branch)

```
AEZEL/
├── app/
│   ├── src/main/
│   │   ├── AndroidManifest.xml
│   │   └── java/com/aezel/vcu/
│   │       ├── MainActivity.kt               # Main Jetpack Compose Entry Activity
│   │       ├── ble/AezelBleManager.kt        # BLE GATT Stack & Telemetry Parser
│   │       ├── model/VehicleStateData.kt     # Vehicle Data Model & Warnings Decoder
│   │       └── ui/
│   │           ├── theme/                    # AEZEL Dark Theme Palette
│   │           └── screens/
│   │               ├── DashboardScreen.kt     # Live Telemetry & Gauges
│   │               ├── RemoteControlScreen.kt # Remote Engine Start & Actuators
│   │               ├── NavigationScreen.kt    # Turn-by-Turn Nav Push Engine
│   │               └── AnalyticsScreen.kt    # Ride Metrics & GPX Logs
│   └── build.gradle.kts
├── .github/workflows/android_build.yml       # GitHub Actions CI/CD APK Build Pipeline
├── build.gradle.kts                          # Root Gradle Configuration
├── settings.gradle.kts                       # Settings File
├── gradlew                                   # Gradle Executable Script
└── docs/android_app_architecture.md          # Technical Specification
```

---

## ⚙️ Building the App

### Option 1: Using GitHub Actions (Automated)
Every `push` or `pull_request` to the `android-app` branch automatically triggers the GitHub Actions CI/CD workflow, compiling the APK and making `app-debug.apk` available for download under **GitHub Actions > Artifacts**.

### Option 2: Building Locally via Gradle
```bash
./gradlew assembleDebug
```
The compiled APK binary will be output at: `app/build/outputs/apk/debug/app-debug.apk`.

---

## 📜 License

Licensed under the **Apache License 2.0**.
