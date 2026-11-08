# Firmware Roadmap — MVP to Production

## Phase 0 — Bench validation (this repo's current state)
- [x] Core managers compiling against mocked/bench sensor inputs
- [x] Main dashboard UI (speed/RPM/gear/trip/fuel/temp/indicators/warnings)
- [x] NVS persistence, SD logging, BLE telemetry contract
- [ ] Unit-test SensorManager's pulse-to-speed/RPM math against a signal
      generator (before ever trusting it on a real wheel)
- [ ] Verify FreeRTOS task timing under load with `uxTaskGetStackHighWaterMark`
      logged from the diagnostics task

## Phase 1 — Installed MVP (rideable, core info only)
- [ ] Wire actual hall/RPM sensors, calibrate wheel circumference
      (`docs/calibration.md`)
- [ ] Validate opto-isolated discrete inputs against real 12V harness
      signals with an oscilloscope — confirm no false triggers from
      ignition-coil noise
- [ ] Confirm safe-shutdown timing: does `IDLE_LINGER_MS` give enough time
      for the last NVS write to complete before board power realistically
      drops (test with a bench supply ramped down, not just ignition-off)
- [ ] On-bike vibration test for 50+ km before trusting the SD card socket
      / connector choices

## Phase 2 — Full dashboard UI
- [x] Build TripInfo, Notifications, Settings screens (same
      `buildXScreen()` pattern as MainDashboard)
- [x] Screen-flow state machine + swipe/button navigation wiring
      (`docs/screen_flow.md`) — rotary quadrature turn-to-navigate still open
- [ ] Ride-mode presets (Eco/City/Touring/Sport/Rain) mapped to concrete
      theme/brightness/warning-threshold table — Settings screen currently
      lets you cycle and select a mode (stored in `VehicleState`), but no
      manager yet reacts to it by changing thresholds/behavior
- [ ] Calibration Wizard UI (fuel curve, wheel size, IMU zero-point) — the
      procedures exist in `docs/calibration.md` but are manual/off-device;
      an on-dash Settings sub-flow to walk through them is still open
- [ ] Persist Settings screen selections (theme, ride mode) to NVS — the
      keys are reserved in `docs/nvs_layout.md` but not yet written on
      selection, so they reset on reboot

## Phase 3 — Analytics & navigation
- [ ] Gear-position inference (RPM/speed-ratio heuristic) if no aftermarket
      gear-position sensor is fitted — flag confidence level in UI, don't
      claim precision the sensor doesn't have
- [ ] Complementary/Kalman filter fusing accel+gyro for real lean-angle
      accuracy (current implementation is accelerometer-only, adequate for
      a dashboard readout, not for cornering-light control)
- [ ] Turn-by-turn navigation engine via BLE companion link (see `docs/navigation_and_telemetry_roadmap.md`):
      - Official Map Integration (No in-app map clones):
        - Android: Adopt [Shr-c0de/navHUD](https://github.com/Shr-c0de/navHUD) pattern
          using `NotificationListenerService` to mirror official Google Maps turn notifications.
        - iOS: Native companion bridge launching official Apple Maps (`maps://?daddr=...`)
          or Google Maps with background CoreBluetooth maneuver updates.
      - ESP32-S3 BLE GATT receiver in `BleManager` decodes packed `NavPacket`
        (maneuver ID, distance, ETA, street name) into `NavigationManager` & `ScreenDashboard`.
- [ ] Hardware GNSS & Trip Telemetry:
      - Integrate [Circuit-Digest/Simple-GPS-Tracker-using-ESP32](https://github.com/Circuit-Digest/Simple-GPS-Tracker-using-ESP32---Visualize-Data-on-Map)
        techniques for u-blox NEO-6M NMEA sentence filtering ($GPRMC/$GPGGA)
        and cross-platform Leaflet/OSM route mapping (works on iOS Safari & Android Chrome).
- [ ] Phone-assisted GNSS fallback (dual Android & iOS):
      - Adapt [and0789/Android-GPS-Bluetooth-Communication-with-ESP32](https://github.com/and0789/Android-GPS-Bluetooth-Communication-with-ESP32)
        concept to stream fused phone GPS coordinates (Android Fused Location / iOS CoreLocation)
        over BLE GATT when onboard hardware GPS fix is lost or module is omitted for budget builds.
- [ ] Cross-Platform Flutter Companion App (`app/` for iOS & Android):
      - Live Cockpit UI: Speed, RPM, battery health, fuel range, and lean angle gauge.
      - Profile Switcher: Instant BLE switching across the 6 pre-configured vehicle presets.
      - Remote Controls: Interlocked hazard flash, horn chirp, and anti-theft immobilizer.
      - Maintenance Trackers: Service countdowns for chain lube (500 km), oil, brake pads, drive belt.
      - Wireless OTA Flasher: Connect to `AEZEL-VCU-AP` and flash firmware updates with one tap.
      - Ride Heatmap / Replay: Synchronize SD GPX files and replay with elevation and lean angle heatmaps.

## Phase 4 — Security & connectivity
- [ ] RFID/NFC/PIN unlock feeding an immobilizer relay (safety-critical —
      needs a fail-safe design so a firmware crash can't lock a rider out
      mid-ride or fail to immobilize when it should)
- [ ] Geofence + unauthorized-movement alert using GpsManager + IMU motion
      detection, SMS/push via companion app relay (ESP32 has no cellular —
      phone does the actual SMS/push send)
- [ ] REST API / MQTT — only worth building once there's a concrete
      consumer (home automation? fleet dashboard?); don't build unused
      surface area

## Phase 5 — CAN bus (only if migrating to a CAN-equipped bike/ECU)
- [ ] Add MCP2515 CAN controller + TJA1050 transceiver module on SPI
- [ ] New `CanBusManager` populates the *same* `VehicleState` fields the
      analog SensorManager currently does (RPM, engine temp, speed) — this
      is the architecture payoff: swapping analog sensors for CAN reads
      touches one new manager file, zero changes to Display/Ride/Notification
- [ ] Feature-flag in `Config.h` to select analog-sensor vs CAN-bus mode at
      build time, or auto-detect CAN bus presence at boot

## Ongoing / never "done"
- [ ] Testing checklist (`docs/testing_checklist.md`) run before every
      firmware release that touches safety-relevant code (warnings, crash
      detection, power management)
- [ ] Watchdog/crash-log review after every test ride
