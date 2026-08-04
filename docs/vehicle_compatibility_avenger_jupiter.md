# AEZEL — Vehicle Compatibility Matrix: Bajaj Avenger 150 (2015) & TVS Jupiter ZX (2017)

This document provides a detailed compatibility analysis for installing the **AEZEL VCU & Cockpit Platform** on two specific popular Indian 2-wheelers:
1. **Bajaj Avenger 150 Street / Cruise (2015 Model)** — 150cc Manual Motorcycle
2. **TVS Jupiter ZX (2017 Model)** — 110cc Automatic CVT Scooter

---

## 📑 Table of Contents

- [1. Bajaj Avenger 150 (2015) Compatibility Matrix](#1-bajaj-avenger-150-2015-compatibility-matrix)
  - [1.1 What WILL Work (Fully Supported)](#11-what-will-work-fully-supported)
  - [1.2 What WILL NOT Work (or Needs Aftermarket Sensors)](#12-what-will-not-work-or-needs-aftermarket-sensors)
  - [1.3 Avenger 150 Harness Wiring Guide](#13-avenger-150-harness-wiring-guide)
- [2. TVS Jupiter ZX (2017) Compatibility Matrix](#2-tvs-jupiter-zx-2017-compatibility-matrix)
  - [2.1 What WILL Work (Fully Supported)](#21-what-will-work-fully-supported)
  - [2.2 What WILL NOT Work (or Not Applicable)](#22-what-will-not-work-or-not-applicable)
  - [2.3 Jupiter ZX Harness Wiring Guide](#23-jupiter-zx-harness-wiring-guide)

---

## 1. Bajaj Avenger 150 (2015) Compatibility Matrix

The 2015 Bajaj Avenger 150 is a carburetted single-cylinder 4-stroke cruiser with a 12V DC battery system, DTS-i ignition coil, manual 5-speed transmission, and mechanical cable speedometer.

```
+---------------------------------------------------------------------------------------+
|                            BAJAJ AVENGER 150 (2015)                                    |
+---------------------------------------------------------------------------------------+
|  POWER: 12V DC Battery      IGNITION: DTS-i Coil Negative    SPEED: Cable Speedo    |
|  GEARS: 5-Speed Manual      FUEL: 10-90Ω Float Sender       BRAKES: Disc / Drum     |
+---------------------------------------------------------------------------------------+
```

### 1.1 What WILL Work (Fully Supported)

| Feature Subsystem | Compatibility | Technical Implementation / Notes |
| :--- | :---: | :--- |
| **12V Power & Deep Sleep** | ✅ 100% | Buck converter powers off battery (+12V permanent). Key switch taps GPIO 48 opto-isolator. Deep sleep draws `< 15 µA`. |
| **Engine RPM Pickup** | ✅ 100% | Tapping primary coil-negative wire off the DTS-i coil through RC filter ($10\text{k}\Omega / 100\text{nF}$) provides 1 pulse/rev timing up to 10,000 RPM. |
| **Wheel Speed & Odometer** | ✅ 100% | Hall effect sensor mounted on rear swingarm reading magnet on rear sprocket bolt ($1.518\text{m}$ circumference calibration). |
| **Fuel Gauge Level** | ✅ 100% | Taps stock tank float sender wire (10–90Ω resistive range) into AEZEL ADC voltage divider for 0–100% fuel bar display. |
| **Turn Signals & High Beam** | ✅ 100% | Opto-isolated inputs tap Left Turn, Right Turn, Neutral Switch, and High Beam harness lines. |
| **3-Phase OEM Animations** | ✅ 100% | Splash screen, 270° tachometer arc sweep, `"188"` segment self-test, indicator bulb test, and goodbye screen. |
| **Smartphone Remote Control** | ✅ 100% | Keyless ignition relay, remote horn pulse, hazard flasher relays, and Find My Bike alarm. |
| **🚀 Remote Engine Start** | ✅ 100% | Enforces Neutral safety interlock (verifies Neutral switch `N` before starter solenoid relay pulse). |
| **GPS & Ride Analytics** | ✅ 100% | Real-time GPS speed, altitude, heading, GPX/CSV logging to SD card, and BLE telemetry. |
| **Smart Lighting & Accents** | ✅ 100% | WS2812B NeoPixel ring welcome pulse, auto-DRL PWM, and emergency hard-braking flasher. |

### 1.2 What WILL NOT Work (or Needs Aftermarket Sensors)

| Feature Subsystem | Compatibility | Reason / Alternative Solution |
| :--- | :---: | :--- |
| **Automotive CAN / OBD-II** | ❌ NOT WORKING | **Reason**: 2015 Avenger 150 is carburetted and has no ECU or CAN bus OBD-II port.<br>**Behavior**: `CanManager` probes, reports inactive, and disables CAN polling without crashing. |
| **Gear Position (1 to 5)** | ⚠️ PARTIAL | **Reason**: Stock Avenger only has a single Neutral switch (displays `N`).<br>**Solution**: Gears 1–5 can be displayed by installing an aftermarket 5-pin gear switch on the shift drum, or using AEZEL's speed-to-RPM calculation heuristic. |

### 1.3 Avenger 150 Harness Wiring Guide
- **Switched +12V Key Wire**: Brown wire inside headlight bucket -> GPIO 48 Opto-Isolator.
- **Coil Negative Wire**: Black/Yellow wire on primary coil -> GPIO 17 RC Filter.
- **Neutral Switch**: Light Green wire near front sprocket cover -> GPIO 35 Opto-Isolator.
- **Left Indicator**: Green wire inside headlight bucket -> GPIO 19 Opto-Isolator.
- **Right Indicator**: Grey wire inside headlight bucket -> GPIO 0 Opto-Isolator.

---

## 2. TVS Jupiter ZX (2017) Compatibility Matrix

The 2017 TVS Jupiter ZX is a 110cc single-cylinder 4-stroke automatic CVT scooter with 12V DC battery system, TCI ignition coil, electric starter, and electric seat lock solenoid.

```
+---------------------------------------------------------------------------------------+
|                             TVS JUPITER ZX (2017)                                     |
+---------------------------------------------------------------------------------------+
|  POWER: 12V DC Battery      IGNITION: TCI Coil Negative     SPEED: Cable Speedo    |
|  TRANSMISSION: Auto CVT     FUEL: 10-90Ω Float Sender       SEAT: Electric Solenoid |
+---------------------------------------------------------------------------------------+
```

### 2.1 What WILL Work (Fully Supported)

| Feature Subsystem | Compatibility | Technical Implementation / Notes |
| :--- | :---: | :--- |
| **12V Power & Deep Sleep** | ✅ 100% | Powers from 12V 4Ah battery. Key switch taps GPIO 48. Deep sleep draws `< 15 µA`. |
| **Engine RPM Pickup** | ✅ 100% | Tapping TCI ignition coil negative wire provides clean 1 pulse/rev timing up to 8,500 RPM. |
| **Wheel Speed & Odometer** | ✅ 100% | Hall effect sensor mounted on front fork reading magnet on front disc/wheel rotor bolt. |
| **Fuel Gauge Level** | ✅ 100% | Taps stock fuel tank sender wire into AEZEL ADC voltage divider for 0–100% display. |
| **🔓 Electric Seat Lock** | ✅ 100% | TVS Jupiter ZX has an electric seat latch solenoid! Taps AEZEL 500ms impulse relay directly for smartphone seat release. |
| **Smartphone Remote Control** | ✅ 100% | Keyless ignition, remote horn pulse, hazard relays, and Find My Bike alarm. |
| **🚀 Remote Engine Start** | ✅ 100% | Scooter remote start (engages starter relay with brake lever switch verification). |
| **GPS & Ride Analytics** | ✅ 100% | Real-time GPS speed, altitude, heading, GPX/CSV logging to SD card, and BLE telemetry. |
| **3-Phase OEM Animations** | ✅ 100% | Splash screen, 270° tachometer arc sweep, `"188"` segment self-test, and goodbye screen. |

### 2.2 What WILL NOT Work (or Not Applicable)

| Feature Subsystem | Compatibility | Reason / Alternative Solution |
| :--- | :---: | :--- |
| **Automotive CAN / OBD-II** | ❌ NOT WORKING | **Reason**: 2017 Jupiter ZX (BS4 Carburetted) has no ECU or CAN bus port.<br>**Behavior**: `CanManager` probes, reports inactive, and disables CAN polling. |
| **Manual Gears (1 to 5)** | ❌ N/A (CVT) | **Reason**: Jupiter ZX is a CVT automatic scooter without manual gears.<br>**Behavior**: Displays permanent Automatic / Neutral mode (`N` or `A`). |
| **Clutch Lever Switch** | ❌ N/A (CVT) | **Reason**: Scooters have no clutch lever (left lever operates rear brake). |

### 2.3 Jupiter ZX Harness Wiring Guide
- **Switched +12V Key Wire**: Red/Black wire under front nose panel -> GPIO 48 Opto-Isolator.
- **Coil Negative Wire**: White/Red wire on TCI coil -> GPIO 17 RC Filter.
- **Electric Seat Solenoid Wire**: Blue/Yellow wire under seat lock -> GPIO 19 Solenoid Relay.
- **Left Indicator**: Orange wire under nose panel -> GPIO 19 Opto-Isolator.
- **Right Indicator**: Light Blue wire under nose panel -> GPIO 0 Opto-Isolator.
