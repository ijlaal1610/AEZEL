# Screen Flow — AEZEL

## Current state (this repo)

All four screens in the cycle are built and wired: `MAIN_DASHBOARD`,
`TRIP_INFO`, `NOTIFICATIONS`, `SETTINGS`. Navigation works via touch swipe
(LVGL gesture detection), the physical MODE button (short press = next,
long press = jump to Settings), and the rotary encoder's press (aliased to
the OK button — see the note on quadrature decode below).

`NAVIGATION` (compass/heading/turn-by-turn) is intentionally **not** in the
build or the cycle yet — it depends on the routing-engine decision flagged
in `docs/roadmap.md` Phase 3, and there was no honest way to ship a fourth
screen for it without either faking data or leaving it blank. Adding it
later is the same `buildXScreen()` pattern as the other three, plus one
more `case` in `goToScreen()`'s switch and one more step in the
`nextScreen()`/`prevScreen()` cycle.

## Implemented state diagram

```
   ┌──────────────┐   ┌───────────────────┐   ┌──────────────┐   ┌────────────────┐
   │ MAIN_DASHBOARD│──►│  TRIP_INFO         │──►│ NOTIFICATIONS │──►│  SETTINGS       │
   │ (speed/RPM/   │◄──│  (trip A/B, odo,   │◄──│ (alert list,  │◄──│  (theme, ride   │
   │  gear/fuel/   │   │   ride timer,      │   │  tap to       │   │   mode,         │
   │  temp/        │   │   avg/max speed,   │   │  acknowledge) │   │   brightness,   │
   │  indicators)  │   │   fuel range/eff., │   │               │   │   HW status)    │
   │               │   │   reset A/B        │   │               │   │                 │
   │               │   │   buttons)         │   │               │   │                 │
   └───────┬───────┘   └────────────────────┘   └───────────────┘   └────────┬────────┘
           │                                                                  │
           └──────────────────────────  cycle wraps  ◄──────────────────────┘
```

Swipe left / MODE short-press moves right through this cycle; swipe right
moves left. It wraps at both ends (Settings → swipe left → Main Dashboard,
and Main Dashboard → swipe right → Settings).

## Navigation input mapping (as implemented)

| Input | Action |
|---|---|
| Swipe left | Next screen (`DisplayManager::nextScreen()`) |
| Swipe right | Previous screen (`DisplayManager::prevScreen()`) |
| Physical MODE button, short press | Same as swipe left |
| Physical MODE button, long press (>1s, `LONG_PRESS_MS`) | Jump directly to Settings from any screen |
| Physical OK button / rotary press | On the Notifications screen: acknowledge the current (oldest unacknowledged) notification |
| Touch tap | Buttons/sliders on Settings and Trip Info respond to direct taps as normal LVGL widgets; individual notification rows are tappable to acknowledge that specific one |

**Not yet implemented:** turning the rotary encoder (quadrature decode on
`PIN_ROTARY_A`/`PIN_ROTARY_B`) doesn't navigate yet — only its press
(`PIN_ROTARY_SW`) does anything, and that's aliased to the OK button
behavior above. Touch swipe and the MODE button already cover full
navigation, so this isn't blocking; adding quadrature decode is the same
ISR-pulse-counting pattern `SensorManager` already uses for speed/RPM.

## Transition rules (implemented in `DisplayManager::tick()`/`nextScreen()`/`prevScreen()`)

- A `CRITICAL` notification (see `NotificationManager`) force-switches to
  `NOTIFICATIONS` the moment it appears, regardless of current screen, and
  both `nextScreen()`/`prevScreen()` refuse to move away from it while any
  `CRITICAL` notification remains unacknowledged — checked fresh on every
  navigation attempt, not just once.
- `Screen::MAIN_DASHBOARD` is always the screen shown on boot
  (`DisplayManager::begin()` calls `goToScreen(Screen::MAIN_DASHBOARD)`
  explicitly after building all four screens) — never resumes mid-navigation
  into Settings.
- Screen transitions animate via `LV_SCR_LOAD_ANIM_MOVE_LEFT/RIGHT`,
  direction chosen by comparing the target screen's position in the cycle
  to the current one (`goToScreen()`'s `forward` calculation) — spatially
  consistent with whichever swipe direction (or MODE press) triggered it.
  The very first screen load at boot skips animation entirely (there's
  nothing to slide away from).

## What's genuinely still open (see `docs/roadmap.md` for the fuller list)

- Rotary quadrature navigation (noted above)
- Persisting the Settings screen's theme/ride-mode selection to NVS (the
  `theme`/`ride_mode` keys are reserved in `docs/nvs_layout.md` but not yet
  written by `DisplayManager` — selections currently reset on reboot)
- The `NAVIGATION` screen itself, once Phase 3's routing-engine decision is made
