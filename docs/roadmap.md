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
- [x] Ride-mode presets (Eco/City/Touring/Sport/Rain) mapped to a concrete
      theme/brightness/warning-threshold/logging table — see
      `include/RideModeProfile.h`, unit-tested in
      `test/native/test_ride_mode_profile.cpp`. Selecting a mode on the
      Settings screen now actually changes the display theme, backlight
      ceiling, fuel/overtemp warning thresholds, and ride-log write
      frequency, not just a label.
- [ ] Calibration Wizard UI (fuel curve, wheel size, IMU zero-point) — the
      procedures exist in `docs/calibration.md` but are manual/off-device;
      an on-dash Settings sub-flow to walk through them is still open
- [x] Persist Settings screen selections (theme, ride mode) to NVS — writes
      immediately on change, restored on boot by `DisplayManager::begin()`
- [x] Maintenance reminders (Service/Tyre/Chain/Insurance/PUC Due) —
      `MaintenanceManager` now actually raises the `SERVICE_DUE`/`TYRE_DUE`/
      `CHAIN_LUBE_DUE`/`INSURANCE_EXPIRING`/`PUC_EXPIRING` flags that
      previously existed but were never wired to anything; see
      `docs/maintenance.md`. Registration/driving-license reminders from
      the original spec use the same date-based pattern but aren't wired
      up yet — would need two more `WarningFlag` bits and two more
      `MaintenanceManager` fields, same shape as insurance/PUC
- [ ] On-dash UI for configuring tyre wear / insurance / PUC dates —
      currently companion-app-only via BLE (`docs/maintenance.md`), same
      "needs a number/date entry widget" gap as the Calibration Wizard above

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
      BLE rather than on-device routing on an ESP32). **Note**: a
      *lighter-weight* version of "the next turn on the dash" now exists —
      `PhoneLinkManager`'s nav-relay banner just displays whatever your
      phone's existing Maps/Waze app is already computing, no on-device
      routing at all. See `docs/phone_link.md`. This item is specifically
      about actual on-device route calculation, which is still unbuilt.
- [x] Phone-mirrored calls/messages/music (`PhoneLinkManager`) — incoming-
      call modal, message previews (routed into the existing
      `NotificationManager` queue), and a music play/pause/skip widget.
      This directly answers the original spec's "Smartphone Integration"
      section (Caller ID, Call Accept/Reject, Message Notifications, Music
      Controls) which had nothing built for it at all until now. Needs a
      companion app to actually forward phone data — see
      `docs/phone_link.md` for the exact contract and what's still that
      app's job versus firmware's.
- [x] Speed-aware "quiet mode" for notification banners — not in the
      original spec by this name, but addresses the same underlying
      concern as several spec items about not overwhelming the rider with
      information at speed. `DisplayPolicyMath::shouldSuppressBanner()`,
      unit-tested to guarantee CRITICAL warnings are never suppressed.
- [ ] Ride heatmap / replay — SD-stored GPX already gives raw data; this is
      a companion-app visualization feature, not firmware

## Phase 4 — Security & connectivity
- [x] Unauthorized-movement alert (wheel motion / lean tilt / GPS drift) —
      `SecurityManager`, arm/disarm from the companion app over bonded BLE,
      sustained buzzer+horn+hazard deterrent. See `docs/security.md` for
      what's implemented (local alarm + BLE notification on next connect)
      versus what a real geofence/instant-push system would still need
      (see the next item).
- [ ] True geofence (alert the moment the bike leaves a boundary,
      regardless of BLE proximity) — `SecurityManager`'s GPS-drift trigger
      only notifies a phone that's in BLE range or reconnects later; an
      actual geofence-with-instant-alert needs either a cellular module on
      the bike or a second always-on relay device, neither of which exists
      in this build. Honesty note carried over from `docs/security.md`.
- [ ] RFID/NFC/PIN unlock feeding an immobilizer relay (safety-critical —
      needs a fail-safe design so a firmware crash can't lock a rider out
      mid-ride or fail to immobilize when it should) — note the *simpler*
      remote lock/unlock via phone already exists, see
      `docs/remote_control.md`'s Tier B; this item is specifically about a
      physical key-free unlock method (badge/fob/PIN pad) as an
      alternative to the phone
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
