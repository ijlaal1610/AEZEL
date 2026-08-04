# AEZEL — Smartphone Remote Control Specification

This document details the **BLE GATT Remote Control system**, JSON command schema, security parameters, and companion mobile application integration for **AEZEL**.

---

## 📑 Table of Contents

- [1. Smartphone Remote Control Architecture](#1-smartphone-remote-control-architecture)
- [2. GATT Command Write API](#2-gatt-command-write-api)
- [3. Full Command Payload Reference](#3-full-command-payload-reference)
- [4. Security, Passkey & Bonding](#4-security-passkey--bonding)
- [5. Companion App Code Snippets (Android & iOS)](#5-companion-app-code-snippets-android--ios)

---

## 1. Smartphone Remote Control Architecture

AEZEL exposes a secure BLE GATT Write Service allowing companion smartphone applications to control motorcycle electrical actuators, keyless ignition, turn signals, hazards, horn, and seat release locks:

```
+------------------------+                        +----------------------------------+
|  Smartphone Remote App |  ─── BLE GATT Write ──> | AEZEL VCU (Core 0 -> Core 1)     |
|  (Android / iOS)       |                        | - Keyless Ignition Relay         |
+------------------------+                        | - Horn Beep & Panic Alarm        |
                                                  | - Hazard & Indicator Relays      |
                                                  | - Seat / Trunk Lock Solenoid     |
                                                  +----------------------------------+
```

---

## 2. GATT Command Write API

- **Service UUID**: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
- **Command Write Characteristic**: `6e400003-b5a3-f393-e0a9-e50e24dcca9e` (Write / Write Without Response)

---

## 3. Full Command Payload Reference

Commands are sent as JSON packets over BLE:

| Action | JSON Command Payload | Description / Hardware Response |
| :--- | :--- | :--- |
| **⚡ Keyless Ignition Toggle** | `{"cmd":"remote_ignition_toggle"}` | Toggles VCU ignition relay (Keyless start/stop) |
| **⚡ Force Ignition ON** | `{"cmd":"remote_ignition_on"}` | Energizes VCU ignition circuit via smartphone |
| **🔑 Force Ignition OFF** | `{"cmd":"remote_ignition_off"}` | Disengages ignition circuit & starts shutdown |
| **📣 Horn Beep** | `{"cmd":"remote_horn_beep"}` | Fires 300ms pulse on 12V Horn relay |
| **🚨 Hazard Flasher** | `{"cmd":"remote_hazard_toggle"}` | Toggles dual hazard flasher relays |
| **◀ Turn Left** | `{"cmd":"remote_indicator_left"}` | Activates left turn signal output |
| **▶ Turn Right** | `{"cmd":"remote_indicator_right"}` | Activates right turn signal output |
| **❌ Indicators OFF** | `{"cmd":"remote_indicator_off"}` | Clears all active turn signals |
| **🔓 Seat / Trunk Release** | `{"cmd":"remote_seat_release"}` | Fires 500ms impulse pulse to electric seat solenoid |
| **📍 Find My Bike** | `{"cmd":"find_bike"}` | Flashes NeoPixel ring red & sounds piezo buzzer alarm |
| **🎨 Change Theme** | `{"cmd":"set_theme","val":"sport"}` | Swaps live dashboard color scheme |
| **🔄 Reset Trip A** | `{"cmd":"reset_trip_a"}` | Clears active Trip A distance counter |
| **🔄 Reset Trip B** | `{"cmd":"reset_trip_b"}` | Clears active Trip B distance counter |

---

## 4. Security, Passkey & Bonding

- **Authentication Requirement**: Remote commands require an encrypted, bonded BLE connection (`ESP_LE_AUTH_REQ_SC_MITM_BOND`).
- **Default Static Passkey**: `161099` (Configurable via NVS).
- **Allow-List Dispatch**: Unrecognized JSON payloads are safely discarded without executing.

---

## 5. Companion App Code Snippets (Android & iOS)

### Android (Kotlin)
```kotlin
fun sendRemoteCommand(commandJson: String) {
    val characteristic = gatt?.getService(UUID.fromString("6e400001-b5a3-f393-e0a9-e50e24dcca9e"))
        ?.getCharacteristic(UUID.fromString("6e400003-b5a3-f393-e0a9-e50e24dcca9e"))
    
    characteristic?.value = commandJson.toByteArray(Charsets.UTF_8)
    gatt?.writeCharacteristic(characteristic)
}

// Example usage:
sendRemoteCommand("{\"cmd\":\"remote_ignition_toggle\"}")
```

### iOS (Swift)
```swift
func sendRemoteCommand(commandJson: String, peripheral: CBPeripheral, characteristic: CBCharacteristic) {
    if let data = commandJson.data(using: .utf8) {
        peripheral.writeValue(data, for: characteristic, type: .withResponse)
    }
}

// Example usage:
sendRemoteCommand(commandJson: "{\"cmd\":\"remote_seat_release\"}", peripheral: peripheral, characteristic: characteristic)
```
