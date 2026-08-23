# Themes — AEZEL

## Current state (this repo)

`DisplayManager::applyTheme()` implements a minimal version: it swaps the
RPM arc's accent color based on `ThemeMode`. The full token table below is
the intended design — extend `applyTheme()` to set each token rather than
just the one accent color once more screens/widgets exist to apply them to
(no point building a 10-row color table when only one widget consumes it).

## How theme selection actually works now (Settings screen + ride modes)

There are two ways `ThemeMode` gets set, and they interact deliberately:

1. **Ride-mode change** (`DisplayManager::applyRideModeProfile()`, called
   when the Settings screen's Ride Mode button is tapped): overwrites
   `_selectedTheme` with that mode's default theme from
   `RideModeProfile.h` — Eco→Light, City→Modern Digital,
   Touring→Classic Analog, Sport→Sport, Rain→Dark, Custom→Custom. This is
   the literal "each mode should modify display theme" behavior from the
   original spec.
2. **Manual theme cycling** (Settings screen's Theme button,
   `onThemeButtonClicked()`): overwrites `_selectedTheme` independently of
   ride mode, persists on its own. This override holds until the **next**
   ride-mode change, which resets theme back to that mode's default.

Both write the same `theme` NVS key (see `docs/nvs_layout.md`), so
whichever happened most recently is what's restored on the next boot.
This isn't hidden complexity — it's the practical answer to "what happens
when the rider wants a specific look but also wants ride modes to mean
something": ride mode sets a sensible default, manual choice is a
temporary override, and there's no ambiguity about which wins because it's
whichever action happened last.

## Design tokens (per theme)

Each `ThemeMode` should define:

| Token | Used for |
|---|---|
| `bgColor` | Screen background |
| `accentColor` | RPM arc, active-state icons, progress bars |
| `textPrimary` | Speed digits, main labels |
| `textSecondary` | Units, subtitles, trip info |
| `warningColor` | Warning banner background |
| `fontPrimary` | Speed digit font (some themes may use a different digit style — e.g. Classic Analog wants a serif/technical font, Neon wants something blockier) |
| `iconStyle` | Outline vs filled icon set |

## Theme definitions (target — not all implemented yet)

| Theme | Background | Accent | Character | Default for ride mode |
|---|---|---|---|---|
| Light | `#F5F5F5` | `#2979FF` | Daylight-optimized, high contrast in direct sun | Eco |
| Dark | `#0A0E14` | `#00D4FF` | Good OLED/IPS contrast at night, low glare | Rain |
| Classic Analog | `#1A1A1A` | `#C9A227` | Needle-and-dial aesthetic even on a digital panel — animated needle sweep on the RPM arc rather than a filled arc | Touring |
| Modern Digital | `#0A0E14` | `#00D4FF` | Clean sans-serif digits, the overall build default | City |
| Minimal | `#000000` | `#FFFFFF` | Monochrome, only essential info shown, everything else hidden until swiped to | — |
| Sport | `#0A0E14` | `#FF1744` | Red accent, tighter RPM redline zone highlighted | Sport |
| Retro | `#1C1408` | `#FF8A00` | Warm amber, mimics old CRT/VFD dashboards | — |
| Neon | `#050505` | `#39FF14` | High-saturation, glow-style widget borders | — |
| Cyberpunk | `#0D0221` | `#F72585` with `#00F5D4` secondary | Dual-accent, angular widget shapes | — |
| Custom | user-defined | user-defined | Free choice via the Settings Theme button, no ride-mode default overwrites it | Custom |
| Animated Theme | cycles | cycles | Slow color-shift over time — novelty theme, lowest implementation priority, not in `ThemeMode` enum yet |
| Automatic Day/Night | switches Light↔Dark | switches | Driven by either `lightLux` ambient sensor crossing a threshold, or GPS-derived sunrise/sunset time once `WeatherManager`/`NavigationManager` exist, not in `ThemeMode` enum yet |

Themes without a "default for ride mode" entry (Minimal, Retro, Neon,
Cyberpunk) are only reachable via manual cycling on the Settings screen —
no ride mode selects them automatically. That's a deliberate scope choice,
not an oversight: the spec listed more themes than there are ride modes to
naturally map them to, and inventing a mapping (e.g. "Neon = ???") would
be arbitrary rather than meaningful.

## Implementation note

Prefer driving `applyTheme()` from a `struct ThemeTokens { ... }` table
indexed by `ThemeMode`, rather than a growing switch-statement per widget —
that keeps adding a new theme a one-row addition instead of touching every
widget's styling code. This repo's current single-switch implementation is
intentionally minimal until more widgets exist to justify the table.

