# Changelog

All notable changes to the AEZEL project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0-alpha] - 2026-08-04

### Added
- **Core Architecture**: Thread-safe `SharedState` mutex-guarded vehicle data model (`DataModel.h`).
- **FreeRTOS Task Graph**: Pinned dual-core scheduling (`CORE_REALTIME` on Core 1, `CORE_CONNECTIVITY` on Core 0).
- **Subsystem Managers**:
  - `SensorManager`: Speed (hall sensor EMA), RPM (coil-negative), ADC (fuel/battery/charge/light), I2C IMU (MPU6050), Barometer (BMP280), 1-Wire DS18B20.
  - `RideManager`: Trip A/B, odometer, ride time, max/avg speed, fuel consumption & range integration.
  - `DisplayManager`: LVGL 8.4 60 FPS dashboard, sweep RPM arc, speed, gear indicator, fuel bar, warning banners, theme engine.
  - `PowerManager`: Ignition state machine (`Active` → `Linger` → `Safe Shutdown` → `Deep Sleep`), `ext0` wake-on-ignition.
  - `NotificationManager`: 16-flag warning bitmask pipeline, prioritized queue, Piezo buzzer chimes.
  - `LightingManager`: DRL PWM auto-brightness, hazard light relay, WS2812B RGB accent lighting animations.
  - `GpsManager`: NEO-6M/8M NMEA parsing, GPS speed cross-checking, RTC auto-sync.
  - `BleManager`: NimBLE GATT telemetry service (2 Hz JSON feed) + GATT write command dispatcher.
  - `StorageManager`: NVS flash settings protection + SD card CSV/GPX ride log exporter.
- **Simulation & Tools**:
  - `diagram.json`: Validated 11-part Wokwi virtual testbench mapped to ESP32-S3 pins.
  - `setup.sh`: Automated one-command environment installation, PlatformIO compilation, and simulation runner.
- **Documentation**: Professional project guides (`docs/bom.md`, `docs/wiring.md`, `docs/power_distribution.md`, `docs/calibration.md`, `docs/roadmap.md`, `docs/testing_checklist.md`, `docs/nvs_layout.md`).
