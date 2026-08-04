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
  companion app (trip reset, find-my-bike hook)
- SD-card ride logging with CSV/GPX export, NVS-backed settings/odometer
  survive power loss
- Ignition-triggered power lifecycle: active → linger → safe-flush → deep
  sleep, wake-on-ignition
- DRL auto-brightness, hazard relay, WS2812 accent lighting with
  welcome/goodbye animation and brake-flash
- FreeRTOS task graph, dual-core pinned (UI/sensors on core 1,
  connectivity/storage on core 0), 8s watchdog

**Architected with a clear extension point but not fully built out** (each
has a manager stub or a documented hook — see `docs/roadmap.md`):
- Turn-by-turn navigation / offline maps / speed-camera alerts
- CAN bus (future expansion — spec below)
- RFID/NFC/fingerprint unlock, geofence, remote lock/immobilizer
- Full multi-screen UI (Trip/Navigation/Notifications/Settings screens —
  Main Dashboard is fully built; others follow the identical
  `buildXScreen()` pattern)
- Cloud backup, REST API, MQTT
- Companion mobile app (BLE contract is defined and stable; app itself is a
  separate codebase)

This is the honest state of a **from-scratch professional build**: a working,
extensible MVP core, not a simulated "everything included" facade.

## Repository layout

```
include/
  Config.h          All pin assignments & hardware constants (edit ONLY here)
  DataModel.h        SharedState — the single, mutex-guarded source of truth
src/
  main.cpp            Boot sequence + FreeRTOS task graph
  managers/           One file pair per subsystem, each independently testable
    SensorManager      Raw sensor acquisition (speed, RPM, fuel, temp, IMU, env)
    RideManager         Distance/time integration, trip/odometer, fuel range
    PowerManager         Ignition lifecycle, sleep states, safe shutdown
    StorageManager        NVS settings + SD ride logs / CSV / GPX export
    LightingManager        DRL, hazard relay, RGB accent, brake-flash
    NotificationManager     Warning→notification pipeline, buzzer
    GpsManager                NMEA parsing, RTC discipline
    BleManager                  Companion-app telemetry + command service
    DisplayManager                LVGL UI, 60fps render loop, theming
data/                 (LittleFS assets: fonts, icons — add as needed)
docs/                 Wiring, BOM, power distribution, roadmap, calibration
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

## Build

```bash
pio run -e esp32-phoenix          # build
pio run -e esp32-phoenix -t upload
pio device monitor
```

Requires a TFT_eSPI `User_Setup.h` matching your display driver (ILI9488,
ST7796, etc.) — copy `include/User_Setup_example.h` (add per your panel) into
the TFT_eSPI library folder or use `-D USER_SETUP_LOADED` with a custom
setup file as PlatformIO build flags.
