# Build & Architecture Notes

Supplementary notes referenced from a few code comments
(`app/build.gradle.kts`, `AezelRepository.kt`, `AezelForegroundService.kt`)
— collected here rather than repeated inline everywhere.

## Why no dependency injection framework

`AezelRepository` is a plain singleton (`getInstance(context)`), not
Hilt/Dagger-managed. The dependency graph in this app is small and mostly
flat: one BLE manager, no swappable implementations, no multi-module
structure. Hilt earns its complexity when there are several
interchangeable implementations to inject (e.g., a fake repository for
tests, multiple data sources) or a large module graph — neither applies
here yet. If the app grows toward needing real dependency injection (e.g.,
a proper offline cache layer, multiple BLE device profiles), that's the
point to revisit this, not before.

## Release build (currently unconfigured)

`app/build.gradle.kts`'s `release` build type has `isMinifyEnabled = false`
and no signing config. Both are deliberately deferred rather than
guessed at:
- **Minification/ProGuard**: `app/proguard-rules.pro` exists but is empty.
  Turning on `isMinifyEnabled = true` without testing against a minified
  build first is a common source of "works in debug, crashes in release"
  bugs (reflection-based code, Compose's runtime, and BLE callback classes
  are all things ProGuard/R8 can mis-strip without the right keep rules).
  Enable it once there's a way to actually test a release build on a
  device, not before.
- **Signing**: needs a real keystore, which isn't something to generate
  blind — see Android's own documentation on generating an upload key when
  you're ready to distribute this beyond your own device/CI artifacts.

## App icon

`app/src/main/res/drawable/ic_launcher_foreground.xml` is a simple
geometric placeholder (an arc + needle, in AEZEL's accent color) — enough
for the project to have a valid, buildable adaptive icon from day one, not
real brand art. Same story for the foreground-service notification's small
icon in `AezelForegroundService.kt`, currently a stock Android Bluetooth
glyph (`android.R.drawable.stat_sys_data_bluetooth`).

## Why Android's own BluetoothGatt API, not a BLE library

Covered in `BleConnectionManager.kt`'s own doc comment, repeated here for
visibility: this app can send lock and remote-start commands to a physical
vehicle. The GATT reconnection/retry behavior of a third-party BLE library
is not something to take on faith for that without being able to fully
audit it. Android's own API is more verbose (see the MTU negotiation /
notification subscription dance in that file) but every line of it is
something this project's own code, not a dependency's.

## Testing without a physical AEZEL dashboard

`app/src/test/` currently covers `VehicleState`'s JSON parsing against
realistic firmware payloads — that's real logic worth testing and doesn't
need hardware. The BLE connection lifecycle itself
(`BleConnectionManager.kt`) isn't unit-tested because it's fundamentally
an integration with Android's Bluetooth stack and a real GATT server
(the firmware) — an instrumented test against a mocked
`BluetoothGattCallback` is possible future work, but the actual
confidence-building step is testing against real firmware hardware once
you have it, per the main firmware repo's own
`docs/incremental_build.md`.
