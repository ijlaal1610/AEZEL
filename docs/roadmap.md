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
- [ ] Build TripInfo, Notifications, Settings screens (same
      `buildXScreen()` pattern as MainDashboard)
- [ ] Screen-flow state machine + swipe/rotary navigation wiring
      (`docs/screen_flow.md` — write once Phase 1 confirms the input HW)
- [ ] Ride-mode presets (Eco/City/Touring/Sport/Rain) mapped to concrete
      theme/brightness/warning-threshold table — currently only the enum
      exists in `DataModel.h`
- [ ] Calibration Wizard UI (fuel curve, wheel size, IMU zero-point)

## Phase 3 — Analytics & navigation
- [ ] Gear-position inference (RPM/speed-ratio heuristic) if no aftermarket
      gear-position sensor is fitted — flag confidence level in UI, don't
      claim precision the sensor doesn't have
- [ ] Complementary/Kalman filter fusing accel+gyro for real lean-angle
      accuracy (current implementation is accelerometer-only, adequate for
      a dashboard readout, not for cornering-light control)
- [ ] Turn-by-turn navigation: offline map tile storage on SD + route
      calculation (this is a substantial subsystem — likely needs a
      phone-side routing engine feeding simplified turn instructions over
      BLE rather than on-device routing on an ESP32)
- [ ] Ride heatmap / replay — SD-stored GPX already gives raw data; this is
      a companion-app visualization feature, not firmware

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
