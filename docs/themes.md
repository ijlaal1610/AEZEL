# Themes — Project Phoenix

## Current state (this repo)

`DisplayManager::applyTheme()` implements a minimal version: it swaps the
RPM arc's accent color based on `ThemeMode`. The full token table below is
the intended design — extend `applyTheme()` to set each token rather than
just the one accent color once more screens/widgets exist to apply them to
(no point building a 10-row color table when only one widget consumes it).

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

| Theme | Background | Accent | Character |
|---|---|---|---|
| Light | `#F5F5F5` | `#2979FF` | Daylight-optimized, high contrast in direct sun |
| Dark | `#0A0E14` | `#00D4FF` | Default — good OLED/IPS contrast at night |
| Classic Analog | `#1A1A1A` | `#C9A227` | Needle-and-dial aesthetic even on a digital panel — animated needle sweep on the RPM arc rather than a filled arc |
| Modern Digital | `#0A0E14` | `#00D4FF` | Current default build target — clean sans-serif digits |
| Minimal | `#000000` | `#FFFFFF` | Monochrome, only essential info shown, everything else hidden until swiped to |
| Sport | `#0A0E14` | `#FF1744` | Red accent, tighter RPM redline zone highlighted |
| Retro | `#1C1408` | `#FF8A00` | Warm amber, mimics old CRT/VFD dashboards |
| Neon | `#050505` | `#39FF14` | High-saturation, glow-style widget borders |
| Cyberpunk | `#0D0221` | `#F72585` with `#00F5D4` secondary | Dual-accent, angular widget shapes |
| Custom User Theme | user-defined | user-defined | Stored in NVS (`theme` key reserved in `docs/nvs_layout.md`), edited via Settings screen color pickers |
| Animated Theme | cycles | cycles | Slow color-shift over time — novelty theme, lowest implementation priority |
| Automatic Day/Night | switches Light↔Dark | switches | Driven by either `lightLux` ambient sensor crossing a threshold, or GPS-derived sunrise/sunset time once `WeatherManager`/`NavigationManager` exist |

## Implementation note

Prefer driving `applyTheme()` from a `struct ThemeTokens { ... }` table
indexed by `ThemeMode`, rather than a growing switch-statement per widget —
that keeps adding a new theme a one-row addition instead of touching every
widget's styling code. This repo's current single-switch implementation is
intentionally minimal until more widgets exist to justify the table.
