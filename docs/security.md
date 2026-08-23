# Security / Anti-Theft — AEZEL

`UNAUTHORIZED_MOVE` existed as a warning flag with a ready-made CRITICAL
notification title since the very first version of this firmware — the
original spec listed "Motion Alarm / Tilt Alarm / Tow Detection" — and
none of it was ever wired to anything. `SecurityManager` is what's
missing.

## Arm / disarm, from your phone

```json
{"cmd": "arm_security"}
{"cmd": "disarm_security"}
```

**Arming is refused unless the bike is stationary with the engine off** —
same interlock pattern as `RemoteControlManager`'s lock command, for the
same reason: arming a moving or running bike doesn't make sense and
refusing it outright avoids ambiguity about what "armed" even means in
that state. The result rides back in telemetry's `cmd_result` field, same
as every other command in this firmware — see `docs/remote_control.md`.

Only a **paired/bonded** phone can arm or disarm — `BleManager::begin()`
already calls `NimBLEDevice::setSecurityAuth(true, true, true)`, so an
unpaired phone in range can't send either command. This matters more here
than for the horn/hazard commands: someone able to disarm your alarm
remotely defeats the whole point of it.

## Three triggers, layered by what hardware you actually have

This was built specifically around the incremental-budget reality covered
in `docs/incremental_build.md` — it's useful starting from the *cheapest*
tier, not just once everything's installed:

| Trigger | Hardware needed | Catches |
|---|---|---|
| Wheel motion | **None beyond Tier 1** (the speed sensor the speedometer already needs) | Bike being pushed, rolled, or lifted-and-spun by hand |
| Lean angle deviation | Tier 4 IMU (`ENABLE_IMU`) | Bike being tilted or lifted without the wheel necessarily turning |
| GPS drift | Tier 3 GPS (`ENABLE_GPS`) | A flatbed tow — wheels never turn, bike never tilts, only position changes |

Arming with only Tier 1 hardware installed still gives you real
protection against the most common scenario (someone pushing the bike
away) — you don't need to wait until you've bought a GPS module and an
IMU before this is worth using.

## What actually happens when it triggers

1. `WarningFlag::UNAUTHORIZED_MOVE` is raised — `NotificationManager`'s
   existing scan-for-newly-raised-flags logic fires its standard one-time
   triple-beep and forces the dashboard to the Notifications screen (see
   `docs/screen_flow.md`'s transition rules), exactly like any other
   CRITICAL warning. No new code was needed for that part — it already
   existed and just needed something to raise the flag.
2. **Car-style alarm** (the actual deterrent): with `ENABLE_REMOTE_HORN`/
   `ENABLE_REMOTE_INDICATORS` (Tier 1.5, see `docs/incremental_build.md`)
   installed, `SecurityManager` chirps the horn in sync with the hazard
   lights — both driven by the same on/off toggle every
   `SECURITY_ALARM_PULSE_MS` (350ms default) so they're locked together,
   the way an actual vehicle alarm reads as deliberate rather than as two
   unrelated things happening near each other. This reuses
   `RemoteControlManager`'s existing `horn()`/`hazard()` actuation rather
   than duplicating relay-driving code here.
3. **Buzzer fallback**: without that Tier 1.5 hardware installed yet, the
   same pulse loop toggles the onboard buzzer instead (Tier 0/1, no extra
   parts) — quieter and far less effective as an actual deterrent, but it
   means arming does *something* before you've bought the relays. This is
   a fallback, not the intended experience — wire up Tier 1.5 as soon as
   budget allows if the alarm matters to you.
4. After `SECURITY_ALARM_DURATION_MS` (30s default), the pulse loop stops
   automatically (so a false trigger doesn't drain the battery or
   alienate the neighborhood all night) but **the CRITICAL warning stays
   raised** until the rider disarms — so returning to a bike that
   triggered and
   auto-quieted still shows what happened, on the dash and in the next
   BLE telemetry packet's `security_state` field.

## Honesty about what this is and isn't

This is a **local alarm + BLE notification**, not a cellular tracker or a
monitored security service. If your phone isn't in BLE range (realistically
10-30m) when the alarm fires, you get the local deterrent — the sustained
buzzer/horn/hazards — but you only learn about it on your phone the next
time it reconnects, not the instant it happens. If you want an instant
push notification regardless of proximity, that requires a cellular or
WiFi-connected relay (e.g. the companion app forwarding a BLE event to a
push service while in range, or a cellular module on the bike itself) —
neither exists in this build. Don't rely on this as a substitute for a
physical lock or comprehensive insurance; it's a deterrent and an
after-the-fact notification, not prevention.

## What's still open

- No on-dash Settings-screen arm/disarm button — phone (BLE) is the only
  interface right now, which fits "control it from your phone" but means
  there's no way to arm it without a phone in hand
- No re-arming after a trigger without an explicit disarm+re-arm cycle —
  after `stopAlarmOutput()` the state returns to `ARMED` (ready to
  re-trigger on continued movement) rather than `DISARMED`, which is
  intentional, but there's no distinction in telemetry between "never
  triggered" and "triggered once, quieted, still armed" beyond the
  `UNAUTHORIZED_MOVE` flag itself
- GPS drift threshold (15m default) hasn't been field-tested against
  normal GPS position jitter while stationary — consumer GPS modules can
  drift several meters on their own even sitting still; this may need
  tuning (or averaging several fixes before capturing the baseline) once
  you have real hardware — see `docs/calibration.md` for the pattern to
  follow for tuning any of this project's thresholds
