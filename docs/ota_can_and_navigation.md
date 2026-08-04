# AEZEL — OTA, CAN Bus & Turn-by-Turn Navigation Specification

This document details three advanced OEM subsystems implemented on the **`gemini`** branch of **AEZEL**:
1. **Wireless Over-The-Air (OTA) Firmware Update Manager** ([`OtaManager`](file:///workspaces/AEZEL/src/managers/OtaManager.h))
2. **Automotive CAN Bus & OBD-II Telemetry Subsystem** ([`CanManager`](file:///workspaces/AEZEL/src/managers/CanManager.h))
3. **Turn-by-Turn Navigation Display & BLE Integration**

---

## 📑 Table of Contents

- [1. Over-The-Air (OTA) Wireless Firmware Updates](#1-over-the-air-ota-wireless-firmware-updates)
- [2. Automotive CAN Bus / OBD-II Subsystem (TWAI)](#2-automotive-can-bus--obd-ii-subsystem-twai)
- [3. Turn-by-Turn Navigation Engine](#3-turn-by-turn-navigation-engine)

---

## 1. Over-The-Air (OTA) Wireless Firmware Updates

The [`OtaManager`](file:///workspaces/AEZEL/src/managers/OtaManager.cpp) enables riders to flash firmware binary updates (`.bin`) wirelessly over Wi-Fi without taking apart their motorcycle dashboard or connecting a USB cable.

### 1.1 Web Server & AP Mode
- **Wi-Fi Access Point**: `"AEZEL-VCU-AP"` (Passkey: `aezel1610`)
- **Default IP Address**: `http://192.168.4.1`
- **Activation Hook**: Triggered remotely via BLE GATT command `{"cmd":"remote_ota_wifi"}` or physical mode button hold.

### 1.2 Dual Flash Partition Swapping
Utilizes ESP32 dual-app partition table (`otadata` / `app0` / `app1`). Uploaded firmware images are written directly to the passive partition, verified for magic bytes, and set as active before triggering an automated reboot.

---

## 2. Automotive CAN Bus / OBD-II Subsystem (TWAI)

The [`CanManager`](file:///workspaces/AEZEL/src/managers/CanManager.cpp) interfaces directly with modern EFI motorcycle networks (KTM, Royal Enfield, Yamaha, Kawasaki, Honda, BMW) over ISO 11898-2 CAN 2.0B.

### 2.1 Hardware Connections
- **ESP32-S3 TWAI Controller Pins**: TX (`GPIO 4`), RX (`GPIO 5`).
- **External Transceiver**: SN65HVD230 / TJA1051 3.3V CAN Transceiver connected to OBD-II CAN-High and CAN-Low lines.
- **Baud Rate**: 500 kbps (Standard Automotive CAN).

### 2.2 Polled OBD-II PIDs

| OBD-II PID | Parameter | Request Hex | Formula / Decoding |
| :--- | :--- | :--- | :--- |
| `0x0C` | Engine RPM | `07 DF 02 01 0C` | $\text{RPM} = \frac{(A \times 256) + B}{4}$ |
| `0x0D` | Vehicle Speed | `07 DF 02 01 0D` | $\text{Speed}_{\text{km/h}} = A$ |
| `0x05` | Coolant Temperature | `07 DF 02 01 05` | $\text{Temp}_{^\circ\text{C}} = A - 40$ |
| `0x11` | Throttle Position | `07 DF 02 01 11` | $\text{Throttle}_{\%} = \frac{A \times 100}{255}$ |

---

## 3. Turn-by-Turn Navigation Engine

Integrates companion smartphone GPS navigation (Google Maps / Mapbox SDK) directly into the AEZEL LVGL 8.4 dashboard.

### 3.1 BLE GATT Payload Format
Companion mobile apps push navigation updates over characteristic `6e400003-b5a3-f393-e0a9-e50e24dcca9e`:

```json
{
  "cmd": "nav_update",
  "dist": 150,
  "turn": 1,
  "street": "MG Road",
  "eta": 12
}
```

### 3.2 Turn Icon Encoding

| Icon Index (`turn`) | Direction | Visual Symbol |
| :---: | :--- | :---: |
| `0` | Continue Straight | ⬆ |
| `1` | Turn Left | ◀ |
| `2` | Turn Right | ▶ |
| `3` | U-Turn | ↩ |
