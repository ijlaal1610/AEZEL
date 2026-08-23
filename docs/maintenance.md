# Maintenance Reminders — AEZEL

This closes a gap that existed since the very first draft of this
firmware: `SERVICE_DUE`, `TYRE_DUE`, `CHAIN_LUBE_DUE`, `INSURANCE_EXPIRING`,
and `PUC_EXPIRING` were defined as warning flags, and
`NotificationManager` already had titles ready for every one of them —
but nothing ever actually raised any of the five. `MaintenanceManager`
is what's missing piece.

## How each item is tracked

| Item | Basis | Default behavior | How to configure |
|---|---|---|---|
| Service | Odometer | Self-schedules: first boot sets due = current odometer + `SERVICE_INTERVAL_KM` (3000 km, `Config.h`) | Settings screen "Mark Serviced" button resets it to current odometer + interval |
| Chain lube | Odometer | Self-schedules the same way, `CHAIN_LUBE_INTERVAL_KM` (500 km) | Settings screen "Mark Chain Lubed" button |
| Tyres | Odometer | **Unconfigured by default** — firmware has no way to guess remaining tyre life on first boot | BLE command `set_tyre_due_km` |
| Insurance | Date | **Unconfigured by default** — firmware has no way to know your policy's expiry | BLE command `set_insurance_due_ts` |
| PUC | Date | **Unconfigured by default** | BLE command `set_puc_due_ts` |

"Unconfigured" means the warning simply never fires for that item — see
`MaintenanceMath::distanceStatus()`/`dateStatus()`, both treat a due value
of `0` as "not configured" and always return `OK`. This was a deliberate
design choice covered by `test/native/test_maintenance_math.cpp`: a
firmware update should never suddenly start nagging about a tyre or an
insurance date it was never told about.

## Why service/chain self-schedule but tyre/insurance/PUC don't

Service and chain-lube intervals are properties of the *bike*, not the
*owner* — a 3000km service interval is a reasonable default for this
class of motorcycle regardless of who's riding it, so self-scheduling on
first boot is safe. Tyre wear depends on how worn the tyres already were
when you installed this dashboard, and insurance/PUC dates are personal
paperwork firmware has zero way to know. Inventing defaults for those
would be actively misleading (a "TYRE_DUE" warning that's just wrong).

## Configuring tyre/insurance/PUC via BLE

These aren't on the Settings screen (no date/number-entry widget built
yet — see `docs/roadmap.md`), so they're set from the companion app:

```json
{"cmd": "set_tyre_due_km", "value": 18000}
{"cmd": "set_insurance_due_ts", "value": 1767225600}
{"cmd": "set_puc_due_ts", "value": 1745020800}
```

`value` for the date-based commands is a Unix epoch timestamp (seconds).
These are pure data-entry commands — no actuator involved, so unlike the
`RemoteControlManager` commands in `docs/remote_control.md`, there's no
interlock to reason about; they route straight to `MaintenanceManager`.

## Date-based checks need a real clock

`MaintenanceManager` won't evaluate `INSURANCE_EXPIRING`/`PUC_EXPIRING`
until `time(nullptr)` reads a plausible epoch (after `Tier 3`'s GPS module
has synced the RTC — see `docs/incremental_build.md`). Without that, a
disconnected RTC reads close to zero, which would otherwise either falsely
flag everything as overdue or falsely clear a real expiry — see the
`PLAUSIBLE_EPOCH_FLOOR` guard in `MaintenanceManager::evaluateAll()`.
Distance-based items (service/chain/tyre) don't have this problem since
the odometer is always real, regardless of whether GPS/RTC is installed.

## What's still open

- No on-dash UI for configuring tyre/insurance/PUC dates — companion-app-only for now
- No "6 months OR 3000km, whichever comes first" combined interval — service/chain are purely distance-based
- No registration/driving-license reminders (the original spec listed these too) — same date-based pattern as insurance/PUC would apply, just not wired up yet; would need two more `WarningFlag` bits and two more `MaintenanceManager` fields
