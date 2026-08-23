# AEZEL — ESP32 Smart Motorcycle Cockpit

Firmware for a modular smart dashboard, scoped for a 2015 Bajaj Avenger 150
but wired so the electrical-noise handling, task architecture, and manager
pattern port to any single-cylinder carbureted or FI motorcycle.

## Scope of this codebase

The original spec listed 300+ dashboard features. Shipping literally all of
them at once produces unmaintainable, untestable code — real OEM dash
software is built the way this repo is: a **small, correct core** plus a
**manager pattern** that makes adding the next feature a matter of writing
one new file, not touching ten existing ones.

**Implemented and working end-to-end** (real sensor math, real persistence,
real UI):
- Speed (hall sensor, pulse-interval, EMA-smoothed) + GPS speed cross-check
- RPM (coil-negative pickup) with 60fps arc gauge
- Gear/neutral, indicators, high-beam, brake, clutch, side-stand, kill-switch inputs
- Trip A/B, odometer, ride timer, average/max speed — persisted to NVS
- Fuel % (analog sender) with self-calibrating range/consumption estimate
- Engine + ambient temperature (DS18B20), battery/charging voltage, IMU lean
  angle + crash heuristic, barometric altitude
- GPS position/speed/heading/altitude + RTC auto-sync (NEO-6M/8M via TinyGPS++)
- Warning/notification engine (16-flag bitmask → prioritized queue → buzzer + banner)
- BLE telemetry service (JSON over NimBLE) + remote command channel for a
  companion app (trip reset, find-my-bike hook, horn/hazard, lock/unlock —
  see below)
- SD-card ride logging with CSV/GPX export, NVS-backed settings/odometer
  survive power loss
- Ignition-triggered power lifecycle: active → linger → safe-flush → deep
  sleep, wake-on-ignition
- DRL auto-brightness, hazard relay, WS2812 accent lighting with
  welcome/goodbye animation and brake-flash
- FreeRTOS task graph, dual-core pinned (UI/sensors on core 1,
  connectivity/storage on core 0), 8s watchdog
- **Full 4-screen dashboard UI**: Main Dashboard, Trip Info (trip A/B,
  odometer, ride timer, avg/max speed, fuel range/efficiency, reset
  buttons), Notifications (tap-to-acknowledge list), Settings (theme +
  ride-mode cycling, brightness slider, maintenance status/actions,
  SD/GPS/BLE status) — with touch swipe, physical MODE-button, and
  OK-button navigation, plus a CRITICAL-notification override that
  force-switches to Notifications and blocks navigating away until
  acknowledged
- **Maintenance reminders** (`MaintenanceManager`) — service and chain-lube
  self-schedule from the odometer with on-dash "mark done" buttons; tyre
  wear and insurance/PUC expiry are configured via BLE and raise the
  existing warning/notification pipeline once due — see
  `docs/maintenance.md`
- **Anti-theft motion/tilt/tow alarm** (`SecurityManager`) — arm/disarm
  from your phone (bonded BLE only), works with just the Tier 1 wheel
  sensor and gets more capable as IMU/GPS are added, sustained
  buzzer+horn+hazard deterrent on trigger — see `docs/security.md`
- **Phone-mirrored calls/messages/music + nav relay + quiet mode**
  (`PhoneLinkManager`) — incoming-call modal with accept/reject, message
  previews routed into the existing notification queue, a music
  play/pause/skip widget, a next-turn banner relayed from your phone's own
  nav app, and speed-aware suppression of non-critical banners while
  riding. Needs a companion app to actually forward this data — the
  firmware side and the exact JSON contract that app must speak are both
  in `docs/phone_link.md`
- **Phone remote control** (horn, hazard/indicator flash, lock/unlock,
  optional remote engine start) with interlocks enforced server-side in
  `RemoteControlManager` — see `docs/remote_control.md` before enabling
  anything past the horn/hazard tier
- **Buy-as-you-go hardware**: every optional sensor/output is behind an
  `ENABLE_*` flag in `Config.h`, defaulted off, so the firmware boots and
  runs correctly with only a subset of the BOM installed — see
  `docs/incremental_build.md`
- **Hardware-free test suite** (`test/native/`) — the speed/RPM/fuel math
  and the per-ride-mode behavior table are both pure C++ with zero Arduino
  dependency (`VehicleMath.h`, `RideModeProfile.h`), unit-tested on a plain
  desktop compiler before they ever touch a real wheel or warning
  threshold. This caught two real bugs during development: a speed-sensor
  design that would've quantized to zero at normal riding speed (fixed by
  widening the calculation window and specifying a multi-magnet sensor —
  see `docs/calibration.md`), and a pure-logic header that had accidentally
  pulled in the entire Arduino/FreeRTOS toolchain through a shared enum
  file (fixed by extracting `include/VehicleEnums.h`).

**Architected with a clear extension point but not fully built out** (each
has a manager stub or a documented hook — see `docs/roadmap.md`):
- Turn-by-turn navigation / offline maps / speed-camera alerts / the
  `NAVIGATION` screen itself (depends on the routing-engine decision in
  `docs/roadmap.md` Phase 3)
- CAN bus (future expansion — spec below)
- RFID/NFC/fingerprint unlock, geofence
- Cloud backup, REST API, MQTT
- Companion mobile app (BLE contract is defined and stable; app itself is a
  separate codebase)

This is the honest state of a **from-scratch professional build**: a working,
extensible MVP core, not a simulated "everything included" facade.

## Repository layout

```
include/
  Config.h          All pin assignments, hardware constants & ENABLE_* feature flags
  DataModel.h        SharedState — the single, mutex-guarded source of truth
  VehicleEnums.h       GearState/RideMode/ThemeMode/WarningFlag — zero hardware dependency
  VehicleMath.h          Pure math (speed/RPM/fuel calcs) — zero hardware dependency
  RideModeProfile.h        Per-ride-mode theme/brightness/warning/logging table — zero hardware dependency
  MaintenanceMath.h          Pure due/overdue logic for service/tyre/chain/insurance/PUC — zero hardware dependency
  SecurityMath.h                Pure GPS-drift/lean-angle detection logic — zero hardware dependency
  DisplayPolicyMath.h              Pure quiet-mode + phone-link staleness logic — zero hardware dependency
src/
  main.cpp            Boot sequence + FreeRTOS task graph
  VehicleMath.cpp       Implementation of the pure math above
  RideModeProfile.cpp     Implementation of the ride-mode table above
  MaintenanceMath.cpp       Implementation of the maintenance logic above
  SecurityMath.cpp             Implementation of the security detection logic above
  DisplayPolicyMath.cpp           Implementation of the quiet-mode/staleness logic above
  managers/           One file pair per subsystem, each independently testable
    SensorManager      Raw sensor acquisition (speed, RPM, fuel, temp, IMU, env)
    RideManager         Distance/time integration, trip/odometer, fuel range
    PowerManager         Ignition lifecycle, sleep states, safe shutdown
    StorageManager        NVS settings + SD ride logs / CSV / GPX export
    LightingManager        DRL, hazard relay, RGB accent, brake-flash
    NotificationManager     Warning→notification pipeline, buzzer
    GpsManager                NMEA parsing, RTC discipline
    BleManager                  Companion-app telemetry + command service
    RemoteControlManager          Phone-command actuation + safety interlocks
    MaintenanceManager              Service/tyre/chain/insurance/PUC reminders
    SecurityManager                    Arm/disarm + motion/tilt/tow theft alarm
    PhoneLinkManager                      Call/message/music/nav data mirrored from the phone
    DisplayManager                          LVGL UI (4 screens + call modal), 60fps render loop, theming
test/native/          Hardware-free unit tests for VehicleMath, RideModeProfile, MaintenanceMath, SecurityMath, DisplayPolicyMath (plain g++, no ESP32 needed)
data/                 (LittleFS assets: fonts, icons — add as needed)
docs/                 Wiring, BOM, power distribution, roadmap, calibration, remote control
```

## Why this architecture

**Single mutex-guarded state, not shared globals.** Every manager reads/writes
`SharedState` through `update()`/`snapshot()`. No manager reaches into
another's internals — DisplayManager never touches a GPIO, SensorManager
never touches LVGL. This is what makes "add CAN bus support without
redesigning the core" actually true: a new `CanBusManager` just writes to
the same `VehicleState` fields the analog sensors currently populate, and
every other manager (display, notifications, BLE, storage) needs zero
changes.

**Two FreeRTOS cores, split by latency sensitivity**, not by feature. Core 1
(`CORE_REALTIME`) holds anything the rider's eyes/ears depend on:
render loop, sensor sampling, warnings, lighting. Core 0
(`CORE_CONNECTIVITY`) holds anything that can legitimately block for
milliseconds on I/O: SD writes, BLE, GPS UART, diagnostics. A slow SD card
or a flaky BLE stack can never cause a dropped display frame.

**NVS for small/frequent, SD for bulk.** Flash has finite write-endurance;
odometer/trip persist every ~10s at most (`StorageManager::FLUSH_INTERVAL_MS`),
not every tick. Full ride tracks go to SD, which doesn't have that
constraint and gives you CSV/GPX export for free.

**Ignition ≠ power.** See `PowerManager` header comment — the ESP32 runs off
a permanently-live buck converter from battery-positive, and ignition is
read as a logic signal on a GPIO, never used to gate the board's actual
power rail. This is standard automotive-electronics practice and avoids
brownout corruption during cranking.

## Hardware safety notes (read before wiring)

- **Never** connect any 12V motorcycle-harness line directly to an ESP32
  GPIO. Every discrete input in `Config.h` (indicators, brake switches,
  ignition, kill switch, starter) must go through an opto-isolator or a
  properly-rated voltage divider clamped with a zener/TVS.
- Battery/charging voltage sensing uses resistive dividers
  (`BATTERY_DIVIDER_RATIO`, `CHARGE_DIVIDER_RATIO` in `Config.h`) — size
  resistors for your actual expected max voltage (motorcycle charging
  systems can spike well above nominal 14V under load-dump conditions; a
  TVS diode across the ADC input is mandatory, not optional).
  See `docs/power_distribution.md`.
- Put a reverse-polarity diode and a TVS diode ahead of the buck converter
  input. Automotive buck converters (not USB phone chargers) handle
  load-dump and cranking-dip far better — see BOM.
- The RPM pickup line carries ignition coil switching noise; the spec's
  opto-isolator + RC low-pass on that input is not cosmetic, it's the
  difference between a clean tach and random RPM spikes corrupting your
  redline warning logic.

## Documentation

- `docs/bom.md` — full bill of materials with component recommendations
- `docs/wiring.md` — wiring diagram description, connector recommendations,
  harness routing
- `docs/power_distribution.md` — power tree, fusing, protection components
- `docs/roadmap.md` — MVP → production firmware roadmap, feature phasing
- `docs/testing_checklist.md` — bench + on-bike test checklist
- `docs/calibration.md` — wheel circumference, fuel-sender curve, IMU
  crash-threshold calibration procedures
- `docs/nvs_layout.md` — NVS/EEPROM key layout
- `docs/screen_flow.md` — UI screen states and navigation
- `docs/themes.md` — theme token system
- `docs/pcb.md` — custom PCB layout recommendations
- `docs/incremental_build.md` — **start here if buying hardware in stages** —
  purchase tiers mapped to `Config.h` feature flags
- `docs/remote_control.md` — **read before enabling any `ENABLE_REMOTE_*`
  flag** — what phone commands do, and the safety interlocks behind each one
- `docs/maintenance.md` — service/tyre/chain/insurance/PUC reminder system:
  what self-schedules, what needs configuring via BLE, and why
- `docs/security.md` — the anti-theft alarm: how arm/disarm works, what
  each trigger needs, and what this honestly is and isn't (not a
  cellular tracker)
- `docs/phone_link.md` — calls/messages/music/nav-relay/quiet-mode: the
  exact BLE JSON contract a companion app needs to speak, and what's
  firmware versus what still needs that (unbuilt) app

## Build

```bash
pio run -e esp32-aezel          # build
pio run -e esp32-aezel -t upload
pio device monitor
```

Requires a TFT_eSPI `User_Setup.h` matching your display driver (ILI9488,
ST7796, etc.) — copy `include/User_Setup_example.h` (add per your panel) into
the TFT_eSPI library folder or use `-D USER_SETUP_LOADED` with a custom
setup file as PlatformIO build flags.
