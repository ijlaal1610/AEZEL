# AEZEL Companion (Android)

The phone-side counterpart to the [AEZEL](../aezel) motorcycle dashboard
firmware. Speaks the exact BLE JSON protocol documented across the
firmware's `docs/remote_control.md`, `docs/maintenance.md`,
`docs/security.md`, and `docs/phone_link.md`.

## Status

This is a complete, from-scratch Kotlin/Jetpack Compose app — real BLE GATT
client, five working screens, a foreground service, and a
`NotificationListenerService` for call/message/music mirroring. It has
**not been built or run on a device by me** — I don't have an Android SDK
or emulator in the environment I wrote this in. GitHub Actions (see
`.github/workflows/android-ci.yml`) is the actual first real build/test
verification this project gets — check the Actions tab after pushing.
If it fails, that's useful signal, not a sign the whole approach is wrong;
treat the first few CI runs as the real "does this compile" step that
would normally happen in Android Studio as you write it.

## What's real and what's a starting point

**Solid, closely matches the firmware:**
- BLE connection lifecycle (`ble/BleConnectionManager.kt`) — scan, connect,
  MTU negotiation (needed because the firmware's JSON telemetry exceeds
  BLE's default 23-byte MTU), notification subscription, command writes
- `data/VehicleState.kt` — field-for-field match with
  `BleManager::publishTelemetry()`, with a real unit test suite
  (`app/src/test/`) checking the parsing against realistic payloads
- Every command in the firmware's protocol docs has a corresponding
  function in `data/AezelRepository.kt`
- Dashboard, Remote (with the two-step confirmation for remote-start that
  `docs/remote_control.md` calls for), Security, Maintenance (including
  tyre/insurance/PUC date entry), and Device/pairing screens

**Real, but with an honestly-documented platform limitation:**
- `service/PhoneActionHandler.kt` — accepting a call works (Android allows
  any app `ANSWER_PHONE_CALLS`); **rejecting/ending a call does not work
  reliably unless this app is set as the default dialer**, which Android
  restricts to system-level permission otherwise. The code attempts it and
  logs a warning rather than silently pretending it succeeded — see that
  file's doc comment.
- `service/NotificationForwardingService.kt` — call detection uses a
  stable Android API (`Notification.CATEGORY_CALL`) and is reliable.
  Message-app detection (WhatsApp/Telegram/SMS) and Google Maps
  navigation-text parsing are both **heuristics** reading whatever those
  apps happen to put in their notification text — not a public API
  contract, and can break on an app update. See that file's ACCURACY NOTE.

**Not built at all:**
- App icon is a simple placeholder vector mark, not real brand art
- No settings persistence (BLE device address isn't remembered between
  launches — you re-scan every time the app restarts; see "Next steps")
- No offline/cached telemetry view
- Release build signing/minification isn't configured (debug builds only
  — see `app/build.gradle.kts`)

## Building

### Via GitHub Actions (recommended first step)
Push this repo to GitHub and the workflow in `.github/workflows/android-ci.yml`
runs automatically, producing a downloadable debug APK artifact. This is
the actual first real compilation check this code gets.

### Locally, in Android Studio
1. Open this folder as a project.
2. Android Studio should recognize the committed Gradle wrapper
   (`gradlew`/`gradle-wrapper.jar`/`gradle-wrapper.properties`) and offer
   to sync automatically.
3. Run on a device or emulator with Bluetooth LE support (most emulators
   don't have working BLE — a real phone is strongly recommended for
   testing the BLE/notification-listener features specifically).

### Locally, from the command line
```bash
./gradlew assembleDebug
# APK lands in app/build/outputs/apk/debug/
```

## Required permissions, and why

| Permission | Why |
|---|---|
| `BLUETOOTH_SCAN` / `BLUETOOTH_CONNECT` (API 31+) or `ACCESS_FINE_LOCATION` (API ≤30) | BLE scanning/connection — the location permission on older Android is a platform requirement for BLE scan, not something this app uses for location itself |
| `POST_NOTIFICATIONS` | The foreground "connected to AEZEL" status notification |
| `ANSWER_PHONE_CALLS` | Lets the dashboard's Accept button actually answer a ringing call |
| Notification Access (granted via Settings, not a runtime prompt) | Required for call/message/music mirroring — see the Device screen's dedicated settings-deeplink button |

## Next steps if you keep building this

- Persist the paired device's BLE address (`SharedPreferences` or
  DataStore) so it reconnects automatically instead of requiring a rescan
  every launch
- Real app icon
- Release signing config + enable `isMinifyEnabled` with tested ProGuard
  rules (`app/proguard-rules.pro` is currently an empty placeholder)
- Auto-reconnect with backoff if the BLE connection drops mid-ride, rather
  than requiring a manual "Scan & Connect" tap
- If message-app/Maps notification parsing proves too fragile in
  practice, consider narrowing scope to just the call-mirror feature
  (which uses a stable API) and dropping the heuristic parts rather than
  maintaining brittle text-matching against apps that can change anytime
