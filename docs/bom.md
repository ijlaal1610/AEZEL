# Bill of Materials — AEZEL

Prices are rough India-market street estimates (Aug 2026) for a single
prototype unit; drop 30-40% at 50+ unit volumes.

| Category | Component | Recommendation | Est. ₹ |
|---|---|---|---|
| MCU | ESP32-S3 module | ESP32-S3-WROOM-1-N8R8 (8MB flash/8MB PSRAM, dual-core, native USB) | 450 |
| Display | 4.3" or 5" IPS TFT, capacitive touch | RGB or SPI panel with ILI9488/ST7796/GC9503 driver, 480x320 or 800x480 | 1400–2600 |
| GPS | u-blox NEO-M8N module | Better multi-constellation lock than NEO-6M, worth the premium | 700 |
| RTC | DS3231 | ±2ppm, far more accurate than DS1307 | 90 |
| IMU | MPU6050 or ICM-42688-P | ICM-42688-P if budget allows — much lower noise for lean-angle/crash | 120–600 |
| Ambient light | BH1750 | I2C, better dynamic range than LDR+ADC | 60 |
| Temp (engine/ambient) | DS18B20 waterproof probe x2 | Stainless probe version for engine-bay mounting | 150 each |
| Barometric | BMP280 | Altitude + pressure | 90 |
| Speed sensor | Reed/Hall proximity sensor | Omron/Honeywell hall-effect, or reuse OEM ABS tone-ring pickup if fitted | 150–400 |
| RPM pickup | Opto-isolated coil-negative tap | PC817 optocoupler + RC filter (build, not buy) | 30 |
| Fuel sender interface | Signal-conditioning board | Op-amp buffer + divider to condition OEM float-resistance sender to 0-3.3V | build |
| SD card | Industrial-grade microSD, 32GB | SLC/pSLC-class (e.g. Transcend/Apacer industrial) — consumer cards fail fast under vibration+heat | 600 |
| Power | Automotive buck converter | Mornsun/XL4015-based, wide-input (6-40V), automotive load-dump rated | 250 |
| Protection | TVS diode | SMBJ18A or similar, on battery line before buck converter | 20 |
| Protection | Reverse-polarity diode | Automotive Schottky, e.g. SS54 | 15 |
| Protection | Automotive blade fuse holder + fuse | Mini blade fuse, 3A, inline on battery-positive feed | 60 |
| Isolation | Opto-isolators | PC817 x8-10 for all discrete harness inputs | 15 each |
| Connectors | Waterproof automotive connectors | JST-VH, Deutsch DT-series, or Molex MX150 for harness runs | 400–1200 total |
| Enclosure | IP65/IP67 ABS or polycarbonate case | Custom or modified off-the-shelf, UV-stabilized | 800–1500 |
| Lighting | WS2812B RGB strip (accent, optional) | 5V, 24-LED ring or strip | 250 |
| Buzzer | Piezo buzzer, automotive-rated | 300 |
| Misc | Wiring harness, heat-shrink, dielectric grease, cable glands | | 500 |
| **Est. total (single unit)** | | | **₹6,500–9,500** |

## Notes

- **Do not** use a generic USB 5V phone-charger buck module — it lacks
  load-dump and reverse-polarity protection and will die (or worse, feed a
  transient into the ESP32) on the first cold-crank.
- If a gear-position sensor isn't present on the stock gearbox (it usually
  isn't on a carbureted 150cc single), the gear indicator falls back to
  Neutral-only detection via the OEM neutral switch — true 1-5 gear display
  requires either an aftermarket gear-position sensor or an RPM+speed-ratio
  inference algorithm (documented as a Phase 3 roadmap item, non-trivial to
  get reliable).
