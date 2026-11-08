# AEZEL Cross-Platform Companion App & Navigation Roadmap (iOS & Android)

This document specifies the complete architecture and implementation roadmap for the **AEZEL Cross-Platform Companion Mobile App** (iOS & Android), **Turn-by-Turn Navigation** using official Google Maps & Apple Maps, **Onboard GNSS Tracking**, and **Phone-Assisted GPS Fallback**.

---

## 1. Executive Summary & Architecture Strategy

Instead of relying on clunky, third-party in-app map clones that drain battery and lack live traffic data, AEZEL pairs with an official **Flutter-based Cross-Platform Companion App** that operates seamlessly on both **iOS** and **Android**.

```mermaid
flowchart TD
    subgraph Mobile Devices (iOS & Android)
        direction TB
        subgraph Official Map Engines
            GMaps[Official Google Maps]
            AMaps[Official Apple Maps]
        end

        subgraph AEZEL Unified Flutter Companion App
            direction TB
            BLE_Core[BLE Connection Manager<br/>(flutter_reactive_ble)]
            CockpitUI[Live Cockpit & Telemetry UI]
            NavBridge[Navigation Bridge Service]
            ProfileMgr[Vehicle Profile Switcher (6 Presets)]
            RemoteCtl[Remote Controls (Hazard, Horn, Lock)]
            TripSync[GPX Trip Log Sync & Leaflet Replay]
            MaintTracker[Maintenance & Wear Reminders]
            OtaFlasher[One-Tap Wi-Fi OTA Flasher]
        end

        GMaps -->|NotificationListenerService (Android)| NavBridge
        AMaps -->|Deep-Link & ANCS Bridge (iOS)| NavBridge
        PhoneGPS[Phone Fused Location] -->|Backup Location Stream| BLE_Core
    end

    subgraph AEZEL Cluster Hardware (ESP32-S3)
        direction TB
        BleMgr[BleManager.cpp (NimBLE GATT)]
        NavMgr[NavigationManager.cpp]
        GpsMgr[GpsManager.cpp]
        State[VehicleState Central DataModel]
        UI[LVGL 8.4 60fps Cluster (ScreenDashboard)]
        SD[StorageManager.cpp (Micro-SD Logs)]
        Relays[Relay Drivers (Starter, Horn, Hazard)]
    end

    BLE_Core <-->|Unified BLE GATT Contract| BleMgr
    BleMgr -->|NavPacket| NavMgr
    BleMgr -->|Phone GPS Stream| GpsMgr
    BleMgr -->|CommandPacket| State
    State -->|TelemetryPacket| BleMgr
    
    NavMgr --> State
    GpsMgr --> State
    State --> UI
    State --> SD
    State --> Relays
```

---

## 2. Companion App Feature Modules

### Module 1: Live Cockpit & Diagnostics Dashboard
* Real-time gauges replicating cluster telemetry at 2–5 Hz:
  * Speedometer & Tachometer with RPM redline alert
  * Battery health: Voltage level, charging line status, and low-battery warning
  * Fuel gauge: Level percentage, remaining distance (km), and average consumption (km/L)
  * Dynamics: Real-time lean angle gauge, pitch, and G-force indicator
  * Temperatures: Dual DS18B20 engine head and ambient temperature readouts

### Module 2: Official Navigation Bridge (Zero In-App Map Clutter)
* **On Android:**
  - Background `NotificationListenerService` (inspired by [`Shr-c0de/navHUD`](https://github.com/Shr-c0de/navHUD)).
  - Rider searches in **official Google Maps** and taps "Start Navigation".
  - The app silently intercepts turn notifications (`"In 300m, turn left on MG Road"`), extracts maneuver arrow ID, distance countdown, and street name, and transmits them to the cluster over BLE.
* **On iOS:**
  - One-tap destination launch directly into **official Apple Maps** (`maps://?daddr=...`) or Google Maps (`comgooglemaps://`).
  - Native iOS CoreBluetooth background central syncs turn-by-turn prompts to the cluster.
* **On Both (Phone-Assisted GPS Fallback):**
  - Adapting [`and0789/Android-GPS-Bluetooth`](https://github.com/and0789/Android-GPS-Bluetooth-Communication-with-ESP32), if onboard hardware GPS fix drops below 4 satellites or enters a tunnel/basement, the app streams fused phone GPS coordinates over BLE to keep the odometer and speedometer calibrated.

### Module 3: Vehicle Profile Switcher (6 Built-In Presets)
* Instant over-the-air switching between AEZEL's 6 vehicle profiles:
  1. **Bike (Direct/Non-OBD)**: 5-Speed Manual, 12,000 RPM, Lean Angle, Side-Stand
  2. **Bike (OBD Hybrid)**: 6-Speed Manual, 14,000 RPM, Throttle/Load bars, Lean Angle
  3. **Scooter (Direct/CVT)**: CVT Automatic, 9,000 RPM, Belt Wear Alert, Tilt Alarm
  4. **Scooter (OBD Hybrid)**: CVT Automatic, 10,000 RPM, OBD Coolant, Throttle %
  5. **Car Manual (OBD-II)**: 5-Speed Manual, 7,000 RPM, Dual G-Force Crash Alarm
  6. **Car Auto (OBD-II)**: Automatic P-R-N-D, 7,000 RPM, Powertrain Load / Coolant

### Module 4: Remote Vehicle Controls & Security
* Safety-interlocked BLE commands:
  * Remote hazard light flash (find-my-bike in parking lot)
  * Horn chirp confirmation
  * Remote engine starter (guarded by neutral & side-stand interlocks)
  * Anti-theft immobilizer relay latch

### Module 5: Trip Logging & GPX Replay
* Wireless synchronization of ride sessions from the cluster's micro-SD card.
* Interactive trip playback on open Leaflet.js / OpenStreetMap (inspired by [`Circuit-Digest/Simple-GPS-Tracker`](https://github.com/Circuit-Digest/Simple-GPS-Tracker-using-ESP32---Visualize-Data-on-Map)):
  * Speed profile graph
  * Lean angle heatmap (highlights aggressive cornering)
  * Elevation and fuel consumption profile

### Module 6: Maintenance & Wear Trackers
* Configurable odometer countdown timers:
  * **Motorcycle**: Chain lubrication (every 500 km), chain tension, engine oil change
  * **Scooter**: CVT drive belt inspection (every 8,000 km), transmission oil
  * **General**: Brake pad thickness, tire replacement intervals

### Module 7: Wireless OTA Firmware Flasher
* Automated check for new AEZEL firmware releases.
* Connects the phone to the cluster's Wi-Fi softAP (`AEZEL-VCU-AP`) and uploads the `.bin` update with a single tap.

---

## 3. Unified BLE GATT Communication Protocol

Communication uses custom 128-bit BLE GATT UUIDs to guarantee low latency and zero cross-talk:

```
AEZEL_SERVICE_UUID:        6E400001-B5A3-F393-E0A9-E50E24DCCA9E
AEZEL_CHAR_TELEMETRY_UUID: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E (Notify, 2-5 Hz)
AEZEL_CHAR_COMMAND_UUID:   6E400003-B5A3-F393-E0A9-E50E24DCCA9E (Write Without Response)
AEZEL_CHAR_NAV_UUID:       6E400004-B5A3-F393-E0A9-E50E24DCCA9E (Write Without Response)
AEZEL_CHAR_GPS_FEED_UUID:  6E400005-B5A3-F393-E0A9-E50E24DCCA9E (Write Without Response)
```

### Packet 1: Navigation Packet (`NavPacket` — 42 Bytes Packed)
```c
struct __attribute__((packed)) NavPacket {
    uint8_t  magic;           // 0x55 (Sync byte)
    uint8_t  maneuver_id;     // 0x00 to 0x0F (16 maneuver icons)
    uint32_t distance_meters; // Distance to next maneuver
    uint32_t eta_seconds;     // Estimated seconds remaining
    uint8_t  progress_pct;    // 0 to 100% route progress
    char     street_name[32]; // Null-terminated UTF-8 street/landmark
    uint8_t  checksum;        // XOR checksum of preceding bytes
};
```

### Packet 2: Phone-Assisted GPS Packet (`GpsFeedPacket` — 20 Bytes Packed)
```c
struct __attribute__((packed)) GpsFeedPacket {
    uint8_t  magic;      // 0xAA
    int32_t  latitude;   // Lat * 1e7
    int32_t  longitude;  // Lng * 1e7
    uint16_t speed_kmh;  // Speed * 10
    int16_t  altitude_m; // Altitude in meters
    uint8_t  accuracy_m; // Horizontal accuracy (HDOP equivalent)
    uint8_t  checksum;   // XOR checksum
};
```

---

## 4. Multi-Phase Implementation Roadmap

### Phase 3A: Firmware BLE Ingestion & LVGL Maneuver HUD (Active)
- [x] Configure dedicated BLE GATT characteristics in `BleManager.cpp`.
- [ ] Implement packed binary deserializers for `NavPacket` and `GpsFeedPacket`.
- [ ] Render 16 LVGL maneuver icons on `ScreenDashboard.cpp` with distance badge and street marquee.
- [ ] Add 15-second timeout fail-safe: automatically dismiss navigation badge if phone link is severed.

### Phase 3B: Cross-Platform Flutter Companion App Core
- [ ] Scaffold Flutter multiplatform project (`app/`) targeting iOS and Android.
- [ ] Integrate `flutter_reactive_ble` with auto-reconnection and background state restoration.
- [ ] Implement Live Cockpit UI using custom high-speed CustomPainter gauges (Dark/Light automotive themes).
- [ ] Implement Profile Switcher, Remote Vehicle Controls, and Maintenance Trackers.

### Phase 3C: Android Notification Listener & Navigation Forwarder
- [ ] Implement Android native method channel for `NotificationListenerService`.
- [ ] Regex parser for multi-lingual Google Maps navigation notifications.
- [ ] Background BLE transmitter streaming `NavPacket` to AEZEL cluster while phone screen is off.

### Phase 3D: iOS Deep-Link Navigation & ANCS Fallback
- [ ] Implement iOS deep-linking to launch official Apple Maps / Google Maps.
- [ ] Implement background `CoreLocation` listener to compute distance countdowns to next waypoint.
- [ ] Implement ESP32 ANCS client compatibility in `BleManager` for direct, zero-app iPhone pairing.

### Phase 3E: Telemetry Logging, GPX Replay & Wireless OTA
- [ ] BLE file transfer protocol to download micro-SD GPX logs to mobile phone storage.
- [ ] Integrated Leaflet.js / OpenStreetMap trip viewer with elevation and lean angle heatmaps.
- [ ] In-app Wi-Fi softAP flasher for one-tap wireless firmware updates.
