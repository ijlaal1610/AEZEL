# Incremental Build Guide — Buy As You Go

The firmware is written so you can flash it with almost nothing installed
and add hardware over time — each addition is one flag flip in
`include/Config.h` plus wiring, not a rewrite. This maps the BOM
(`docs/bom.md`) into purchase tiers and shows exactly what flag(s) each
tier unlocks.

## How the flags work

Every optional peripheral is gated by an `ENABLE_*` flag at the top of
`Config.h`, defaulted to `0` (off). With a flag off:
- That manager's `begin()` skips touching the pins/bus for that part entirely
- Its task either isn't created (GPS) or safely no-ops (SD, IMU, barometer, fuel)
- Nothing else in the firmware crashes, hangs, or needs to know — `SharedState`
  fields for missing sensors just stay at their default (0 / false) and the
  dashboard shows them as such

This means **you can flash and bench-test the firmware today with just an
ESP32 dev board and a display** — no wheel, no fuel tank, no GPS. You'll
see 0 km/h, 0 RPM, "N/A" temps, and that's expected and correct, not a bug.

## Tier 0 — Bench / bring-up (what you can test right now)

**Buy:** ESP32-S3 dev board, any SPI/parallel TFT display (even a basic
ILI9341 breakout works for bring-up — swap for the nicer panel later).

**Flags:** everything OFF except `ENABLE_SD_CARD` and `ENABLE_BLE` (both
default on, and SD cards are cheap enough to just include from day one).

**What works:** boot sequence, FreeRTOS task graph, the dashboard UI
rendering (with placeholder/zero values), BLE telemetry connects and shows
those zero values live, NVS persistence (odometer/trip survive a reboot
even though they're not accumulating real distance yet).

**This is also exactly what `test/native/` and Wokwi simulation validate**
before you've bought anything — see the earlier conversation on testing
without hardware.

## Tier 1 — Core ride data (~₹1,200-1,800 incremental)

**Buy:** hall speed sensor, opto-isolator (RPM pickup + first few discrete
inputs — indicators, neutral, brake), reverse-polarity diode + TVS + fuse
for the power input, automotive buck converter.

**Flags:** none new to flip — speed/RPM/discrete inputs were never behind
a flag (they're the sensors `SensorManager` always reads; a disconnected
hall sensor just reads 0 pulses, which is safe and correct with nothing
wired).

**What works:** real speed, RPM, trip/odometer accumulation, indicator/
neutral/brake/kill-switch readouts, ignition-triggered power lifecycle.
This is the point where the dashboard is genuinely "on the bike and
working," even though temp/fuel/battery still read placeholder values.

## Tier 1.5 — Cheap phone-control wins (~₹200-400 incremental)

**Buy:** one relay for horn, two for indicators (or a 4-channel relay board).

**Flags:** `ENABLE_REMOTE_HORN=1`, `ENABLE_REMOTE_INDICATORS=1`

**What works:** horn-honk and hazard-flash from the phone app ("find my
bike"). See `docs/remote_control.md` — these are the lowest-risk remote
commands, worth doing early since they're cheap and immediately useful.

## Tier 2 — Environment & electrical (~₹700-1,000 incremental)

**Buy:** 2x DS18B20 probes, fuel-sender signal-conditioning board, BH1750
ambient light sensor.

**Flags:** `ENABLE_ONEWIRE_TEMP=1`, `ENABLE_FUEL_SENDER=1`,
`ENABLE_AMBIENT_LIGHT=1`

**What works:** engine/ambient temp, fuel %/range/consumption, DRL
auto-brightness instead of fixed. Run the fuel-sender calibration
(`docs/calibration.md`) once this is wired.

## Tier 3 — Location (~₹700)

**Buy:** NEO-M8N GPS module.

**Flags:** `ENABLE_GPS=1`

**What works:** GPS speed cross-check, position/heading/altitude, RTC
auto-sync, ride-log GPS points (feeds the CSV/GPX export that's already
implemented and waiting for real coordinates).

## Tier 4 — Dynamics, cosmetics, security (~₹1,000-2,500 incremental)

**Buy:** MPU6050/ICM-42688-P IMU, BMP280, WS2812B accent strip, immobilizer relay.

**Flags:** `ENABLE_IMU=1`, `ENABLE_BAROMETER=1`, `ENABLE_RGB_ACCENT=1`,
`ENABLE_REMOTE_IMMOBILIZER=1`

**What works:** lean angle, crash heuristic (calibrate thresholds first —
see `docs/calibration.md`), altitude/pressure, welcome/goodbye lighting
animation, brake-flash, and remote lock/unlock from the phone.

## Tier 5 — Remote start (~₹150 incremental, but read the doc first)

**Buy:** one more relay for the starter circuit.

**Flags:** `ENABLE_REMOTE_STARTER=1`

**Read `docs/remote_control.md` in full before enabling this one** — it's
cheap to wire but is the highest-consequence flag in the whole project,
and that doc explains exactly what interlocks are and aren't implemented.

## Suggested order if budget is the constraint

Tier 0 → Tier 1 → Tier 1.5 → Tier 2 → Tier 3 → Tier 4 → (Tier 5 only if
you actually want it). This order front-loads "the dashboard does its core
job" before "the dashboard has nice-to-haves," and puts the one genuinely
risky feature last, after everything else has proven stable on the bike.
