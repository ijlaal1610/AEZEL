# AEZEL — BLE Telemetry Protocol & GATT Command API

This document provides an exhaustive specification of the Bluetooth Low Energy (BLE) interface, GATT service layout, JSON schemas, and companion mobile application integration for **AEZEL**.

---

## 📑 Table of Contents

- [1. BLE Service Architecture](#1-ble-service-architecture)
- [2. GATT Service & Characteristic Definitions](#2-gatt-service--characteristic-definitions)
- [3. Telemetry Notification JSON Schema](#3-telemetry-notification-json-schema)
- [4. GATT Command Write API](#4-gatt-command-write-api)
- [5. Security, Passkey & Bonding](#5-security-passkey--bonding)
- [6. Companion App Implementation Snippets](#6-companion-app-implementation-snippets)

---

## 1. BLE Service Architecture

AEZEL uses **NimBLE-Arduino v1.4.x**, a low-RAM, high-efficiency BLE stack running on **Core 0 (`CORE_CONNECTIVITY`)**. It allows companion smartphone applications (Android / iOS) to receive live telemetry at 2 Hz and issue remote control commands without interrupting the 60 FPS display rendering on Core 1.

```
+-------------------+             BLE GATT Telemetry (2 Hz Notify)          +-----------------------+
|  AEZEL ESP32-S3   | ----------------------------------------------------> | Companion Mobile App  |
|  (GATT Server)    | <---------------------------------------------------- | (Android / iOS)       |
+-------------------+              GATT Command Write (Allow-Listed)        +-----------------------+
```

---

## 2. GATT Service & Characteristic Definitions

### Base UUID: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`

| Entity | UUID | Permissions | Description |
| :--- | :--- | :--- | :--- |
| **Telemetry Service** | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` | Read | Main VCU BLE Service |
| **Telemetry Notify Char** | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | Notify / Read | Broadcasts 2 Hz JSON vehicle state |
| **Command Write Char** | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` | Write / Write W/O Resp | Receives JSON control commands |

---

## 3. Telemetry Notification JSON Schema

Broadcast every 500ms over characteristic `6e400002-...`:

### Sample Payload:
```json
{
  "spd": 65,
  "rpm": 5200,
  "gear": 3,
  "fuel": 82,
  "batt": 13.8,
  "eng_t": 88,
  "amb_t": 31,
  "odo": 14250,
  "trip_a": 42.5,
  "warn": 0,
  "lat": 18.520432,
  "lon": 73.856741,
  "lean": 14.2
}
```

### Field Definitions:

| Key | Data Type | Units | Description |
| :--- | :---: | :---: | :--- |
| `spd` | Integer | km/h | Current ground speed |
| `rpm` | Integer | RPM | Engine rotational speed |
| `gear` | Integer | Index | Gear position (0=Neutral, 1–5=Gears) |
| `fuel` | Integer | % | Tank fuel level (0 to 100%) |
| `batt` | Float | Volts | Main battery voltage (e.g. 13.8V) |
| `eng_t` | Integer | °C | Engine head temperature |
| `amb_t` | Integer | °C | Ambient air temperature |
| `odo` | Float | km | Lifetime vehicle odometer |
| `trip_a` | Float | km | Current Trip A distance |
| `warn` | Integer (Bitmask) | Flags | Active warning bitmask (0 = No Warnings) |
| `lat` | Double | Degrees | Current GPS Latitude |
| `lon` | Double | Degrees | Current GPS Longitude |
| `lean` | Float | Degrees | Dynamic lean angle (IMU Roll) |

---

## 4. GATT Command Write API

Commands are sent as JSON objects to write characteristic `6e400003-...`.

### Supported Commands:

#### 1. Reset Trip A
```json
{"cmd": "reset_trip_a"}
```

#### 2. Reset Trip B
```json
{"cmd": "reset_trip_b"}
```

#### 3. Find My Bike (Flashes hazard lights & sounds buzzer)
```json
{"cmd": "find_bike"}
```

#### 4. Change UI Theme
```json
{"cmd": "set_theme", "val": "sport"}
```
*(Valid values: `"cyan"`, `"sport"`, `"neon"`, `"amber"`)*

---

## 5. Security, Passkey & Bonding

- **Pairing Mode**: Passkey Display / Keyboard input (`ESP_LE_AUTH_REQ_SC_MITM_BOND`).
- **Default Static Passkey**: `161099` (Configurable via NVS).
- **Encrypted Characteristic Access**: Command writes require an encrypted, bonded connection.

---

## 6. Companion App Implementation Snippets

### Android (Kotlin / BleManager)
```kotlin
val commandChar = gatt.getService(UUID.fromString("6e400001-b5a3-f393-e0a9-e50e24dcca9e"))
    ?.getCharacteristic(UUID.fromString("6e400003-b5a3-f393-e0a9-e50e24dcca9e"))

fun resetTripA() {
    val jsonCmd = "{\"cmd\":\"reset_trip_a\"}".toByteArray(Charsets.UTF_8)
    commandChar?.value = jsonCmd
    gatt.writeCharacteristic(commandChar)
}
```

### iOS (Swift / CoreBluetooth)
```swift
func resetTripA(peripheral: CBPeripheral, characteristic: CBCharacteristic) {
    let jsonString = "{\"cmd\":\"reset_trip_a\"}"
    if let data = jsonString.data(using: .utf8) {
        peripheral.writeValue(data, for: characteristic, type: .withResponse)
    }
}
```
