# AEZEL Branch Comparison: `main` vs `gemini`

Date: 2026-08-05

Compared archives:

- `C:\Users\Sumair\Downloads\AEZEL-main.zip`
- `C:\Users\Sumair\Downloads\AEZEL-gemini.zip`

Extracted comparison folders:

- `C:\Users\Sumair\aezel_compare_work\AEZEL-main`
- `C:\Users\Sumair\aezel_compare_work\AEZEL-gemini`

## Executive Summary

`AEZEL-main` is the more conservative and internally consistent branch. It focuses on a modular ESP32 motorcycle dashboard/VCU architecture with compile-time feature flags, safer remote-control routing through `RemoteControlManager`, testable vehicle math, and an incremental hardware build strategy.

`AEZEL-gemini` is the more ambitious branch. It adds OTA firmware update support, CAN/OBD-II telemetry, BLE navigation payload handling, smartphone remote-control command expansion, and a stronger simulated/demo experience. However, several of these features are only partially integrated or introduce regressions:

- New `CanManager` and `OtaManager` files exist, but neither manager is started from `main.cpp`.
- `OtaManager` can be enabled by BLE, but its HTTP server is not ticked unless its task is running.
- `CanManager` hardcodes GPIO 4 and 5, conflicting with existing rotary and SD pins.
- `gemini` removes the compile-time `ENABLE_*` feature flags from `Config.h`, while other files still reference those flags.
- `gemini` bypasses `RemoteControlManager` for BLE actuator commands, weakening the safety boundary from `main`.
- `gemini` changes speed sensing from 4 pulses/rev with a 200 ms accumulation window back to 1 pulse/rev with 50 ms calculations, likely making speed readings noisy.
- Navigation state fields are added, but display rendering appears incomplete.

Recommended direction: use `main` as the base and selectively port the useful `gemini` features into it, preserving `main`'s safety architecture, feature flags, math helpers, and task wiring discipline.

## Diff Size

Source comparison summary:

- `23` files changed
- `830` insertions
- `355` deletions
- `gemini` has `67` files
- `main` has `61` files

New files in `gemini`:

- `docs/claude_vs_antigravity_comparison.md`
- `docs/ota_can_and_navigation.md`
- `src/managers/CanManager.cpp`
- `src/managers/CanManager.h`
- `src/managers/OtaManager.cpp`
- `src/managers/OtaManager.h`

## Build Verification Status

PlatformIO was installed successfully:

```text
PlatformIO Core, version 6.1.19
```

Build verification was attempted but not completed:

- A parallel build attempt caused a PlatformIO package-install race under `C:\Users\Sumair\.platformio\packages`.
- A sequential build attempt for `AEZEL-main` timed out after three minutes without useful compiler output.

So this report is based on static source comparison, not a confirmed successful firmware build.

## Changed Files Overview

Files changed or added in `gemini`:

```text
README.md
docs/bom.md
docs/calibration.md
docs/claude_vs_antigravity_comparison.md
docs/nvs_layout.md
docs/ota_can_and_navigation.md
docs/preview/index.html
docs/smartphone_remote_control.md
include/Config.h
include/DataModel.h
platformio.ini
src/main.cpp
src/managers/BleManager.cpp
src/managers/CanManager.cpp
src/managers/CanManager.h
src/managers/DisplayManager.h
src/managers/LightingManager.cpp
src/managers/OtaManager.cpp
src/managers/OtaManager.h
src/managers/RideManager.cpp
src/managers/SensorManager.cpp
src/managers/SensorManager.h
src/managers/StorageManager.cpp
```

## Architecture Difference

### `main`

`main` is organized around guarded, incremental hardware bring-up:

- Optional peripherals are controlled through `ENABLE_*` flags in `include/Config.h`.
- `main.cpp` only creates certain tasks when their feature flags are enabled.
- `SensorManager`, `StorageManager`, and `LightingManager` compile out optional hardware support when disabled.
- Remote phone commands are routed through `RemoteControlManager`.
- `VehicleMath` centralizes speed, RPM, odometer, fuel, and smoothing math.
- Native tests exist for `VehicleMath`.

This branch is more suitable for real hardware bring-up where not every peripheral is connected on day one.

### `gemini`

`gemini` moves toward a broader VCU/connected dashboard feature set:

- Adds CAN/OBD-II telemetry manager.
- Adds Wi-Fi AP based OTA firmware upload manager.
- Adds BLE navigation command parsing.
- Adds more smartphone remote commands.
- Adds Adafruit NeoPixel dependency.
- Expands documentation and demo preview features.

However, it removes or weakens some of `main`'s control points:

- Feature flags are removed from `Config.h`.
- Several optional subsystems are now always included or always initialized.
- Remote command handling is moved into `BleManager` instead of `RemoteControlManager`.
- Some new systems are present as files but not started at runtime.

## Configuration Changes

File: `include/Config.h`

`main` defines feature flags:

```cpp
#define ENABLE_GPS
#define ENABLE_IMU
#define ENABLE_BAROMETER
#define ENABLE_AMBIENT_LIGHT
#define ENABLE_SD_CARD
#define ENABLE_RGB_ACCENT
#define ENABLE_BLE
#define ENABLE_ONEWIRE_TEMP
#define ENABLE_FUEL_SENDER
#define ENABLE_TOUCHSCREEN
#define ENABLE_REMOTE_HORN
#define ENABLE_REMOTE_INDICATORS
#define ENABLE_REMOTE_IMMOBILIZER
#define ENABLE_REMOTE_STARTER
```

`gemini` removes those definitions entirely.

This is a major compatibility issue because `RemoteControlManager.cpp` still contains `#if ENABLE_REMOTE_HORN`, `#if ENABLE_REMOTE_STARTER`, and similar guards. In the C preprocessor, undefined identifiers in `#if` evaluate as `0`, so the branch may still compile, but the configuration contract is broken. The docs still tell users to toggle flags that no longer exist.

`gemini` also changes actuator pin definitions:

`main`:

```cpp
PIN_OUT_HORN_RELAY        28
PIN_OUT_LEFT_IND_RELAY    29
PIN_OUT_RIGHT_IND_RELAY   30
PIN_OUT_IMMOBILIZER_RELAY 31
PIN_OUT_STARTER_RELAY     32
```

`gemini` removes those remote relay pins and adds:

```cpp
PIN_OUT_STARTER_RELAY   19
PIN_OUT_IGNITION_RELAY  20
```

This breaks the existing `RemoteControlManager` design for horn, indicators, and immobilizer, because its expected pin definitions are gone.

Speed calibration also changes:

- `main`: `HALL_PULSES_PER_REV = 4`
- `gemini`: `HALL_PULSES_PER_REV = 1`

That change directly affects speed calculation stability.

## Main Task Wiring

File: `src/main.cpp`

`main` includes and starts `RemoteControlManager`:

- Includes `managers/RemoteControlManager.h`
- Calls `RemoteControlManager::instance().begin()`
- Creates a `Remote` FreeRTOS task

`gemini` removes all of that.

`gemini` also removes feature-gated task creation:

- GPS task is always created.
- BLE task is always created.
- `ENABLE_GPS` and `ENABLE_BLE` guards are removed.

Important omission: although `gemini` adds `CanManager` and `OtaManager`, `main.cpp` does not include them, call `begin()`, or create their tasks.

Runtime consequence:

- CAN telemetry will not run.
- OTA server request handling will not run unless manually ticked elsewhere.
- Remote safety monitoring from `RemoteControlManager::tick()` will not run.

## BLE Behavior

File: `src/managers/BleManager.cpp`

BLE advertised device name changes:

- `main`: `AEZEL`
- `gemini`: `Phoenix Cockpit`

Telemetry changes:

- `main` publishes `cmd_result` from `RemoteControlManager::lastResultString()`.
- `gemini` removes `cmd_result`.

This is a real behavior regression for the phone app. In `main`, refused commands can be reported back to the app. In `gemini`, failures are mostly silent or converted to dashboard notifications.

Command handling changes:

`main` supports commands through `RemoteControlManager`:

- `reset_trip_a`
- `reset_trip_b`
- `find_bike`
- `horn_on`
- `horn_off`
- `hazard_on`
- `hazard_off`
- `indicator_left_on`
- `indicator_left_off`
- `indicator_right_on`
- `indicator_right_off`
- `lock`
- `unlock`
- `remote_start`
- `remote_stop`

`gemini` replaces that with direct BLE command handling:

- `reset_trip_a`
- `reset_trip_b`
- `find_bike`
- `remote_ignition_toggle`
- `remote_ignition_on`
- `remote_ignition_off`
- `remote_start_engine`
- `remote_horn_beep`
- `remote_hazard_toggle`
- `remote_indicator_left`
- `remote_indicator_right`
- `remote_indicator_off`
- `remote_seat_release`
- `nav_update`
- `remote_ota_wifi`

The good part: `gemini` adds navigation and OTA commands.

The bad part: actuator logic is now in `BleManager`, not in `RemoteControlManager`. That contradicts the safety rule documented in `docs/remote_control.md`, which says `BleManager` should never touch actuator behavior directly.

## Remote Control Safety

This is one of the most important differences.

### `main`

`main` uses `RemoteControlManager` as the single control point for phone-driven physical actions. It includes:

- Feature gates for each remote actuator class.
- Moving/stationary checks.
- Engine-running checks.
- Neutral checks.
- Side-stand checks.
- Kill-switch checks.
- Result reporting through `lastResultString()`.

The design is conservative: BLE parses commands, but physical action decisions live elsewhere.

### `gemini`

`gemini` keeps `RemoteControlManager` files in the tree, but removes them from `main.cpp` and stops using them in `BleManager.cpp`.

`remote_start_engine` in `BleManager` checks neutral and kill switch, then mutates shared state:

- Sets ignition on.
- Sets starter active.
- Sets engine running.
- Sets RPM to simulated idle.

It does not actually call `RemoteControlManager::remoteStart()`.

It also does not use the existing side-stand check, moving check, remote-start timeout, or result telemetry path.

Conclusion: `gemini` has a more feature-rich command list but a weaker safety architecture.

## CAN/OBD-II Addition

New files:

- `src/managers/CanManager.h`
- `src/managers/CanManager.cpp`

What `gemini` adds:

- ESP32-S3 TWAI driver setup.
- 500 kbps CAN timing.
- Accept-all CAN filter.
- OBD-II response parsing for ECU response ID `0x7E8`.
- PID `0x0C`: engine RPM.
- PID `0x0D`: vehicle speed.
- PID `0x05`: engine coolant temperature.
- Broadcast OBD-II polling through ID `0x7DF`.

Problems:

- `CanManager` is not included or started in `main.cpp`.
- CAN pins are hardcoded in `CanManager.cpp`:

```cpp
#define CAN_TX_PIN GPIO_NUM_4
#define CAN_RX_PIN GPIO_NUM_5
```

These conflict with `Config.h`:

```cpp
PIN_ROTARY_A = 4
PIN_SD_CS    = 5
```

- The comment says it polls RPM, speed, coolant temperature, and throttle, but the transmit code only sends an RPM request for PID `0x0C`.
- It parses speed/coolant if received, but does not actually request those PIDs.
- There is no feature flag such as `ENABLE_CAN`.
- There is no detection strategy for vehicles without CAN beyond whether `twai_start()` succeeds. On a carbureted vehicle, the TWAI controller may still start even though no ECU exists.

Conclusion: good starting point, incomplete integration.

## OTA Firmware Update Addition

New files:

- `src/managers/OtaManager.h`
- `src/managers/OtaManager.cpp`

What `gemini` adds:

- Wi-Fi soft AP named `AEZEL-VCU-AP`.
- Password `aezel1610`.
- Embedded HTTP server on port 80.
- HTML upload form.
- `/update` POST endpoint.
- Uses Arduino `Update` library.
- Reboots after successful upload.
- BLE command `remote_ota_wifi` calls `OtaManager::instance().enableWifiAp()`.

Problems:

- `OtaManager::taskEntry()` is never created in `main.cpp`.
- `OtaManager::tick()` is not called from any other manager.
- Therefore `otaServer.handleClient()` will not run continuously.
- No authentication beyond the Wi-Fi AP password.
- AP password is hardcoded.
- No timeout or auto-disable behavior.
- No battery/ignition safety guard before firmware upload.
- No feature flag such as `ENABLE_OTA`.

Conclusion: the OTA implementation is useful but incomplete. It needs task wiring and safety/authentication hardening before relying on it.

## Navigation Addition

Files changed:

- `include/DataModel.h`
- `src/managers/BleManager.cpp`
- `src/managers/DisplayManager.h`
- `docs/ota_can_and_navigation.md`
- `docs/preview/index.html`

`gemini` adds these fields to `VehicleState`:

```cpp
bool     navActive
uint32_t navDistanceMeters
uint8_t  navTurnIcon
char     navStreetName[32]
uint16_t navEtaMinutes
```

BLE accepts:

```json
{
  "cmd": "nav_update",
  "dist": 150,
  "turn": 1,
  "street": "MG Road",
  "eta": 12
}
```

Problems:

- `DisplayManager.h` declares `buildNavigationScreen()` and nav LVGL object pointers.
- I found no corresponding implementation of `buildNavigationScreen()` in `src/managers/DisplayManager.cpp`.
- I found no display refresh logic for `navActive`, `navDistanceMeters`, `navStreetName`, or `navEtaMinutes`.

Conclusion: navigation data plumbing starts in `gemini`, but the firmware UI appears incomplete.

## Sensor Manager Changes

Files:

- `src/managers/SensorManager.cpp`
- `src/managers/SensorManager.h`

`main` gates optional sensor libraries:

- OneWire/DallasTemperature behind `ENABLE_ONEWIRE_TEMP`.
- Adafruit MPU6050 behind `ENABLE_IMU`.
- BMP280 behind `ENABLE_BAROMETER`.
- Fuel sender logic behind `ENABLE_FUEL_SENDER`.

`gemini` removes those guards.

Consequence:

- The firmware now expects these libraries and hardware code paths to be present even for builds that do not use those peripherals.
- This undermines the incremental hardware approach documented in the repo.

Speed/RPM behavior:

`main` accumulates speed/RPM pulses over 200 ms before calculating. The code comments explain why: a low pulse count wheel sensor is unstable if sampled every 50 ms.

`gemini` removes:

```cpp
_lastSpeedRpmCalcMs
SPEED_RPM_CALC_WINDOW_MS = 200
```

and calculates speed/RPM every tick using `dt`.

Likely consequence:

- At normal riding speeds, a 1 pulse/rev hall setup may produce 0 or 1 pulses per tick.
- This creates jumpy speed readings.
- It weakens the regression protection described in `test/native/test_vehicle_math.cpp`.

Math behavior:

`main` uses `VehicleMath` helper functions:

- `pulsesToKmh`
- `pulsesToRpm`
- `dividerVoltage`
- `emaUpdate`

`gemini` inlines these formulas back into `SensorManager`.

That reduces testability.

## Ride Manager Changes

File: `src/managers/RideManager.cpp`

`main` uses `VehicleMath`:

- `speedToMeters`
- `recalibrateKmPerPercent`
- `fuelRangeKm`
- `kmPerLiter`

`gemini` removes the include and inlines these calculations.

The formulas are broadly similar, but this is still a regression in maintainability because `VehicleMath` exists specifically so pure calculations can be tested outside Arduino/ESP32.

## Storage Manager Changes

File: `src/managers/StorageManager.cpp`

`main`:

- Uses NVS namespace `aezel`.
- Gates SD support behind `ENABLE_SD_CARD`.
- Allows NVS-only operation when SD is not installed.
- Uses GPX creator string `AEZEL`.

`gemini`:

- Changes NVS namespace to `phoenix`.
- Removes `ENABLE_SD_CARD` guards.
- Always includes SD/SPI code.
- Changes GPX creator string to `ProjectPhoenix`.

Implications:

- Existing persisted settings under namespace `aezel` will not be read by `gemini`.
- This may look like settings/odometer reset after flashing.
- SD card is no longer optional at compile time.
- Branding becomes inconsistent: many docs still say AEZEL while runtime strings say Phoenix/ProjectPhoenix.

## Lighting Manager Changes

File: `src/managers/LightingManager.cpp`

`main` keeps NeoPixel support behind `ENABLE_RGB_ACCENT` from `Config.h`.

`gemini` adds:

```cpp
#define ENABLE_RGB_ACCENT 1
```

directly inside `LightingManager.cpp`.

Consequences:

- RGB accent is forced on regardless of central configuration.
- This contradicts the comment that it is a one-line toggle in `Config.h`.
- `platformio.ini` adds the NeoPixel library dependency.

This is useful for the richer demo, but worse for minimal hardware builds.

## Display Manager Changes

File: `src/managers/DisplayManager.h`

`gemini` adds declarations for a turn-by-turn navigation screen:

- `buildNavigationScreen()`
- `_screenNav`
- `_labelNavTurnIcon`
- `_labelNavDistance`
- `_labelNavStreet`
- `_labelNavEta`

Problem:

- I found no matching implementation in the source tree.

If `buildNavigationScreen()` is called anywhere in a future edit without implementation, that will cause linker failure. If it is not called, navigation UI is simply not functional.

## PlatformIO Changes

File: `platformio.ini`

Environment name changes:

- `main`: `[env:esp32-aezel]`
- `gemini`: `[env:esp32-phoenix]`

Dependency added in `gemini`:

```ini
adafruit/Adafruit NeoPixel@^1.12.0
```

This matches the forced NeoPixel behavior in `LightingManager.cpp`.

## Documentation Changes

`gemini` adds or expands documentation for:

- OTA updates.
- CAN/OBD-II.
- Turn-by-turn navigation.
- Smartphone remote control.
- Demo preview remote controls.
- Claude vs Antigravity comparison.

However, some documentation in `gemini` no longer matches the code:

- Docs still describe `RemoteControlManager` as the central actuator safety path, but `BleManager.cpp` bypasses it.
- Docs describe `ENABLE_*` flags, but `Config.h` no longer defines them.
- Docs imply CAN probing and deactivation behavior that the code does not fully implement.
- Docs imply navigation dashboard rendering, but implementation appears missing.

## Preview HTML Changes

File: `docs/preview/index.html`

`gemini` expands the browser preview with smartphone controls:

- Remote start engine.
- Keyless ignition.
- Horn beep.
- Hazards.
- Find my bike.
- Seat lock.
- Turn navigation simulation.
- Wi-Fi OTA update simulation.

This is useful for demonstrating the concept, but it is more of a UX/demo expansion than proven embedded behavior.

## Brand Naming Differences

`main` consistently uses AEZEL.

`gemini` introduces Phoenix naming:

- BLE name: `Phoenix Cockpit`
- NVS namespace: `phoenix`
- GPX creator: `ProjectPhoenix`
- PlatformIO environment: `esp32-phoenix`

This may be intentional rebranding, but if the project is still AEZEL it creates confusion and migration issues.

## Risk Assessment

### High Risk

Remote-control safety regression:

- `main` routes actuator commands through `RemoteControlManager`.
- `gemini` bypasses it and mutates shared state in BLE.
- Result telemetry is removed.
- Remote safety monitor task is no longer started.

CAN pin conflicts:

- `CanManager` uses GPIO 4 and 5.
- Existing config uses GPIO 4 for rotary A and GPIO 5 for SD CS.

OTA incomplete runtime wiring:

- BLE can enable the AP/server.
- No task is started to run `otaServer.handleClient()`.

Feature flags removed:

- Optional hardware is no longer cleanly configurable.
- Docs and code are out of sync.

Speed calculation regression:

- 200 ms accumulation window removed.
- 4 pulses/rev changed to 1 pulse/rev.
- Likely noisier speedometer behavior.

### Medium Risk

Navigation incomplete:

- State fields and BLE parsing exist.
- UI implementation appears missing.

NVS namespace changed:

- Existing stored settings under `aezel` will not carry over to `phoenix`.

Sensor dependencies always included:

- Minimal builds may require libraries/hardware paths they do not need.

Hardcoded OTA credentials:

- Better than open AP, but still weak and fixed.

### Low Risk

Documentation and preview expansion:

- Useful as concept material.
- Needs alignment with actual firmware before treating as implementation truth.

## Recommended Merge Strategy

Use `AEZEL-main` as the base branch.

Port from `gemini` selectively:

1. Add `OtaManager`, but introduce `ENABLE_OTA`, start its task from `main.cpp`, add timeout/authentication, and avoid hardcoded production credentials.
2. Add `CanManager`, but move CAN pins into `Config.h`, add `ENABLE_CAN`, fix pin conflicts, and poll all claimed PIDs.
3. Add navigation state fields and BLE `nav_update`, but implement the display screen and refresh path before calling it complete.
4. Keep `RemoteControlManager` as the only actuator-control layer. BLE should call it, not duplicate or bypass it.
5. Restore `cmd_result` telemetry so the phone app knows why commands were accepted or rejected.
6. Preserve `VehicleMath` and its native tests. Do not inline tested formulas back into hardware managers.
7. Keep `ENABLE_*` flags in `Config.h` and update docs to match actual defaults.
8. Decide whether the product is AEZEL or Phoenix, then make naming consistent.

## Branch Choice

For a real motorcycle hardware build:

Use `main`.

For harvesting experimental ideas:

Use `gemini` as a feature source, not as the direct firmware base.

For a demo presentation:

`gemini` has a richer story because of OTA/CAN/navigation/smartphone controls, but the firmware implementation should be corrected before claiming those features are production-ready.

## Bottom Line

`gemini` contains valuable feature ideas, especially OTA, CAN, and navigation. But `main` is currently the stronger engineering base. The safest path is not to replace `main` with `gemini`; it is to merge the useful `gemini` additions back into `main` carefully, while preserving `main`'s modular build flags, safety boundaries, and tested math layer.
