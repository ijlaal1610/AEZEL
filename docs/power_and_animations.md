# AEZEL — Power Architecture & Animation Guide

This document details how AEZEL handles vehicle power distribution, ignition state detection, shutdown protection, and visual boot/shutdown animations.

---

## 📑 Table of Contents

- [1. Automotive Power Architecture](#1-automotive-power-architecture)
- [2. Ignition Sense & Detection](#2-ignition-sense--detection)
- [3. Power State Machine & Deep Sleep](#3-power-state-machine--deep-sleep)
- [4. Boot & Shutdown Animations](#4-boot--shutdown-animations)
- [5. Interactive Animation Preview](#5-interactive-animation-preview)

---

## 1. Automotive Power Architecture

In standard consumer electronics, turning off a power switch physically disconnects the battery from the circuit. In automotive and motorcycle electronics (OEM VCU/ECU design), doing so would cause instant memory corruption and prevent graceful shutdown.

AEZEL uses a **permanently live power architecture**:

```
+12V Battery Positive  ---> [Fuse] ---> [TVS Diode] ---> [Buck Converter] ---> 5V / 3.3V to ESP32
                                                                                     │
Ignition Key Switch    ---> [Opto-Isolator] ────────────────────────────────────────> GPIO 48 (Logic Sense)
```

1. **Permanent Battery Rail**: The ESP32-S3 and its high-efficiency automotive buck converter remain permanently connected to **Battery Positive (+12V)** through a fused, reverse-polarity protected line.
2. **Ignition as a Logic Input Only**: Turning the ignition key ON or OFF does **NOT** interrupt power to the buck converter. Instead, it toggles a logic signal read by **GPIO 48 (`PIN_IN_IGNITION`)**.

---

## 2. Ignition Sense & Detection

The ignition signal from the motorcycle harness passes through an opto-isolator (or Zener-clamped resistor divider) to isolate high-voltage alternator transients from the ESP32:

- **Key ON**: GPIO 48 receives `3.3V` (`HIGH`).
- **Key OFF**: GPIO 48 drops to `0V` (`LOW`).

Because the board stays powered by the battery when the key is turned OFF, the ESP32 remains fully operational to run flash protection algorithms and display shutdown animations.

---

## 3. Power State Machine & Deep Sleep

Managed in [`PowerManager.cpp`](file:///workspaces/AEZEL/src/managers/PowerManager.cpp):

```
[KEY ON] ---> ACTIVE ---> [KEY OFF] ---> LINGER (5s) ---> SAFE_SHUTDOWN ---> DEEP_SLEEP (<15µA)
                 ^                              |
                 |--- [RE-KEYED WITHIN 5s] -----|
```

### 3.1 Lifecycle States

1. **`ACTIVE`**: Full operation (60 FPS rendering, sensor sampling, BLE, GPS, SD logging).
2. **`LINGER` (5-Second Window)**: When key-off is detected, `PowerManager` enters a 5-second waiting phase. If the rider accidentally stalled or turned the key back on immediately, the system resumes `ACTIVE` instantly without a full reboot.
3. **`SAFE_SHUTDOWN`**: If ignition remains `LOW` after 5 seconds:
   - Triggers `LightingManager::playGoodbyeAnimation()`.
   - Triggers `DisplayManager::showGoodbyeScreen()`.
   - Flushes dirty NVS settings (odometer, trip values).
   - Safely closes open SD card GPX/CSV log files.
4. **`DEEP_SLEEP`**: Configures hardware wake interrupt and puts ESP32 into deep sleep:
   ```cpp
   esp_sleep_enable_ext0_wakeup(PIN_IN_IGNITION, HIGH);
   esp_deep_sleep_start();
   ```

### 3.2 Deep Sleep Battery Drain
In **Deep Sleep**, the ESP32-S3 consumes less than **10–15 µA** (microamps)—meaning it would take years to discharge a standard motorcycle battery. When the key is turned back ON, GPIO 48 goes `HIGH`, triggering an instant hardware wake-up reset.

---

## 4. Boot & Shutdown Animations

### 4.1 Welcome / Boot Animation
- **WS2812B NeoPixel Ring**: [`LightingManager::playWelcomeAnimation()`](file:///workspaces/AEZEL/src/managers/LightingManager.cpp#L66-L75) sweeps/fades the 24-pixel RGB LED accent ring from brightness `0` to `255` in steps of `15` every 20ms in electric blue (`RGB(0, 120, 255)`).
- **TFT Display Fade-In**: [`DisplayManager::begin()`](file:///workspaces/AEZEL/src/managers/DisplayManager.cpp#L30-L56) initializes LVGL 8.4 and loads the main dashboard with a 200ms screen fade transition (`LV_SCR_LOAD_ANIM_FADE_ON`).

### 4.2 Goodbye / Shutdown Animation
- **TFT Display Goodbye Screen**: [`DisplayManager::showGoodbyeScreen()`](file:///workspaces/AEZEL/src/managers/DisplayManager.cpp#L239-L249) displays a centered, high-contrast screen reading **`"See you next ride"`** for ~1.2 seconds.
- **WS2812B NeoPixel Fade-Out**: [`LightingManager::playGoodbyeAnimation()`](file:///workspaces/AEZEL/src/managers/LightingManager.cpp#L77-L86) fades out accent ring brightness from `255` down to `0` over 350ms and clears all pixels before sleep entry.

---

## 5. Interactive Animation Preview

An interactive web simulator is available at [`docs/preview/index.html`](file:///workspaces/AEZEL/docs/preview/index.html).

### Preview Features:
- **🔑 Ignition Key Toggle**: Test turning key ON (triggers Welcome NeoPixel sweep + Dashboard fade-in) and turning key OFF (triggers Linger delay, Goodbye screen, NeoPixel fade-out, and Deep Sleep state).
- **⚡ Speed & Tachometer Controls**: Drag throttle slider to simulate 270° sweep RPM arc gauge and calculated gear shifting (`N`, `1`–`5`).
- **🎨 Live Theme Selector**: Preview `Modern Digital`, `Sport Mode`, `Neon Mode`, and `Retro Analog` color themes.
