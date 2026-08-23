# 📱 AEZEL Smart Motorcycle Cockpit — Cross-Platform Companion App (Android & iOS)

A unified, high-performance **Flutter & Dart** cross-platform companion application for the **AEZEL ESP32-S3 Smart Motorcycle Cockpit & VCU**. 

This single codebase targets **both Android and iOS** devices simultaneously with 100% feature parity, 60 FPS animated tachometer arc gauges, cross-platform Bluetooth LE GATT communication, and automated GitHub Actions CI/CD workflows.

---

## 🌟 Key Features Across iOS & Android

* 📱 **Single Unified Codebase (Dart & Flutter)**: Build and maintain one application for both iPhone (iOS) and Android smartphones.
* 🏎️ **Handlebar Cockpit Fullscreen Kiosk Mode**: Immersive full-screen mode hiding iOS/Android status bars and gesture navigation bars for dedicated handlebar phone mounting.
* ⚡ **270° Animated Tachometer Arc & Speedometer**: Smooth 60 FPS custom painter gauge with **Neon Redline Shift Flash** (> 9,500 RPM) and center speed readout.
* 📡 **Cross-Platform Bluetooth LE Stack (`flutter_blue_plus`)**: Autodiscovers `"AEZEL VCU"` / `"Phoenix Cockpit"`, subscribes to GATT notifications at 2 Hz, and dispatches JSON control commands.
* 🚀 **Remote Engine Start**: One-touch engine starter with **Neutral Safety Interlocks** (`gear == 'N'`).
* ⚡ **Full Actuator Control Grid**: Keyless Ignition, Horn Pulse, Hazard Flasher, Electric Seat Release Solenoid, Find My Bike Alarm, Security PIN manager, and OTA trigger.
* 🗺️ **Automated Background GPS Bridge**: Listens for background turn directions from Google Maps / Apple Maps and streams turn arrows, distance, and street names to the VCU display.
* ⚙️ **Automated CI/CD Workflows**: Builds both **Android APKs** (`app-release.apk`) and **iOS App Bundles** (`Runner.app`) on GitHub Actions!

---

## 📂 Project Architecture

```
aezel_companion (cross-platform-app branch)
├── lib/
│   ├── main.dart                      # App entry point, Provider, Kiosk controls & gesture drawer
│   ├── models/
│   │   └── vehicle_state.dart         # 21-parameter GATT JSON parser & warning decoder
│   ├── services/
│   │   └── ble_service.dart           # FlutterBluePlus GATT BLE Manager (iOS & Android)
│   └── ui/
│       ├── theme/
│       │   └── aezel_theme.dart       # Cyberpunk Dark Cockpit color palette
│       ├── widgets/
│       │   ├── tachometer_gauge.dart  # 270° CustomPainter Tach Arc & Redline Flash
│       │   └── quick_drawer.dart      # Slide-out quick navigation drawer overlay
│       └── screens/
│           ├── dashboard_screen.dart   # 60 FPS Gauge & Metric Grid
│           ├── remote_control_screen.dart # Actuators & Remote Engine Start
│           ├── navigation_screen.dart  # Automated GPS Maps Bridge
│           ├── analytics_screen.dart   # Max Speed, Avg Speed, Odometer, GPX export
│           └── settings_screen.dart    # Speedo toggle, Focus mode, PIN security
├── android/
│   └── app/src/main/AndroidManifest.xml # Android BLE & Location permissions
├── ios/
│   └── Runner/Info.plist              # iOS NSBluetoothAlwaysUsageDescription & Location keys
└── .github/workflows/flutter_build.yml # GitHub Actions CI/CD for Android APKs & iOS Runner bundle
```

---

## 🛠️ How to Build & Run

### 1. Run on Device / Emulator
```bash
# Fetch Flutter packages
flutter pub get

# Run on connected Android or iOS device
flutter run
```

### 2. Build Release Artifacts
```bash
# Build Android Release APK
flutter build apk --release

# Build iOS App Bundle (Unsigned)
flutter build ios --release --no-codesign
```

---

## ⚙️ GitHub Actions CI/CD

This repository branch includes a dedicated GitHub Actions workflow (`.github/workflows/flutter_build.yml`) that automatically compiles:
1. 🤖 **Android APKs**: Uploaded as artifact `aezel-flutter-android-apks` (`app-debug.apk` & `app-release.apk`).
2. 🍎 **iOS App Bundle**: Uploaded as artifact `aezel-flutter-ios-bundle` (`Runner.app`).

You can also trigger builds manually anytime using the **Run workflow** button under the **Actions** tab on GitHub!
