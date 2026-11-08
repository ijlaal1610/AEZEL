# NVS Memory Layout — AEZEL

`StorageManager` opens a single `Preferences` namespace, `"aezel"`. Keys
below 15 characters (ESP32 NVS key-length limit) — this table is the
authoritative list; add new keys here whenever `StorageManager` grows one so
nobody duplicates a key by accident.

| Key | Type | Written by | Meaning |
|---|---|---|---|
| `odo_km` | float | `StorageManager::flushAll()` | Lifetime odometer, km |
| `tripA_km` | float | `StorageManager::flushAll()` | Trip A distance, km |
| `tripB_km` | float | `StorageManager::flushAll()` | Trip B distance, km |
| `<key>_km` | uint32 | `saveMaintenanceRecord()` | Odometer value at which a maintenance item is due (key = e.g. `svc`, `tyre`, `chain`) |
| `<key>_ts` | uint32 | `saveMaintenanceRecord()` | Epoch timestamp at which a maintenance item is due (for date-based reminders like insurance/PUC) |

## Reserved keys (Phase 2+, not yet written by code)

| Key | Type | Purpose |
|---|---|---|
| `wheel_circ` | float | Runtime-calibrated wheel circumference (replaces the `Config.h` compile-time constant once the Calibration Wizard exists) |
| `fuel_curve` | blob (JSON) | Fuel-sender lookup table points from `docs/calibration.md` procedure |
| `theme` | uint8 | Persisted `ThemeMode` selection across reboots |
| `ride_mode` | uint8 | Persisted `RideMode` selection |
| `ble_bond_*` | managed by NimBLE internally, not this namespace | BLE pairing bond storage (NimBLE uses its own NVS namespace) |

## Design rules

- **Odometer/trip only get a fresh NVS write on `flushAll()`**, called
  every `FLUSH_INTERVAL_MS` (10s) by `StorageManager::tick()`, and
  explicitly by `PowerManager::requestSafeShutdown()` before deep sleep.
  Do not add a code path that calls `_prefs.put*()` directly on every
  sensor tick — that burns flash write-endurance for no benefit, since
  losing up to 10 seconds of trip distance on an abrupt power loss is an
  acceptable tradeoff already documented in the main README.
- Namespace stays singular (`"aezel"`) rather than one-namespace-per-
  manager — simpler backup/restore/factory-reset story (wipe one namespace,
  not N), and NVS overhead per namespace isn't free on a resource-constrained
  MCU.
- Maintenance-record keys are prefixed by a short mnemonic (`svc`, `tyre`,
  `chain`, `ins`, `puc`) chosen by the caller — keep these under ~10 chars
  so `"<key>_km"`/`"<key>_ts"` stay within the 15-char NVS key limit.
