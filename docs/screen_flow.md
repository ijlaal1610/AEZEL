# Screen Flow — Project Phoenix

## Current state (this repo)

Only `MAIN_DASHBOARD` is built (`DisplayManager::buildMainDashboard()`).
`DisplayManager::goToScreen()` has the switch statement structured to add
the rest — each follows the identical pattern:

```cpp
void DisplayManager::buildTripInfoScreen() {
    _screenTripInfo = lv_obj_create(nullptr);
    // ... widgets ...
}
```
then a case in `goToScreen()`:
```cpp
case Screen::TRIP_INFO:
    lv_scr_load_anim(_screenTripInfo, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
    break;
```

## Intended state diagram

```
                    ┌───────────────────┐
        swipe R ───►│  NOTIFICATIONS     │
        ◄─ swipe L  │  (alert list,      │
                    │   acknowledge)     │
                    └─────────┬──────────┘
                              │ swipe R / rotary
                              ▼
   ┌──────────────┐   ┌───────────────────┐   ┌──────────────┐
   │  SETTINGS     │◄─►│  MAIN_DASHBOARD    │◄─►│  TRIP_INFO    │
   │  (calibration,│   │  (speed/RPM/gear/  │   │  (trip A/B,   │
   │   theme, ride │   │   fuel/temp/       │   │   avg/max,    │
   │   mode, units)│   │   indicators)      │   │   ride timer) │
   └──────────────┘   └─────────┬──────────┘   └──────────────┘
        long-press               │ swipe L / rotary
        MODE button               ▼
                          ┌───────────────────┐
                          │  NAVIGATION        │
                          │  (compass/heading, │
                          │   GPS speed, ETA   │
                          │   once routing      │
                          │   exists — Phase 3) │
                          └───────────────────┘
```

## Navigation input mapping

| Input | Action |
|---|---|
| Swipe left / rotary CW | Next screen (Dashboard → Trip → Navigation → loop) |
| Swipe right / rotary CCW | Previous screen |
| Tap / rotary press | Select / acknowledge current notification |
| Physical MODE button, short press | Same as swipe left (glove-friendly alternative) |
| Physical MODE button, long press (>1s) | Jump directly to Settings from any screen |
| Physical OK button | Confirm/select within Settings submenus |

## Transition rules

- A `CRITICAL` notification (see `NotificationManager`) force-switches to
  `NOTIFICATIONS` regardless of current screen and current swipe input,
  and blocks navigation away until acknowledged — the rider must not be
  able to swipe past a crash-detected or engine-overtemp alert unnoticed.
- `Screen::MAIN_DASHBOARD` is always the screen shown on wake from deep
  sleep / ignition-on — never resume mid-navigation into Settings, which
  would be disorienting and, for a moving vehicle, unsafe.
- Screen transitions use `LV_SCR_LOAD_ANIM_MOVE_LEFT/RIGHT` to match swipe
  direction (spatial consistency — swiping left should visually feel like
  moving left), except the forced-notification jump, which uses a fade to
  avoid implying a "swipe" happened that didn't.

## Build order (matches `docs/roadmap.md` Phase 2)

1. `TRIP_INFO` — reuses fields already in `VehicleState`, no new sensor work
2. `NOTIFICATIONS` — reuses `NotificationManager::current()`/queue, mostly a
   list-view widget
3. `SETTINGS` — needs the Calibration Wizard sub-flow designed first (see
   `docs/calibration.md`), so this is the more involved of the three
4. `NAVIGATION` — deferred to Phase 3, depends on routing-engine decision
