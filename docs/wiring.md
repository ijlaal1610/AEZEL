# Wiring — AEZEL

This describes the wiring diagram in words + a connection table, since a
true schematic belongs in KiCad (see `docs/pcb.md` for the PCB
recommendation) rather than ASCII art. Pin numbers below match
`include/Config.h` exactly — if you rewire, change them there, not here.

## Signal chain overview

```
Motorcycle harness (12V domain)
        │
        ▼
Opto-isolator bank (PC817 x N)      ← physical isolation boundary
        │  (3.3V logic domain from here on)
        ▼
ESP32-S3 GPIO
```

```
Sensors (5V/3.3V native or conditioned)
  Hall speed sensor ──────────────► GPIO18 (PIN_SPEED_HALL, pull-up, FALLING edge)
  RPM pickup (via opto) ──────────► GPIO17 (PIN_RPM_PICKUP, pull-up, FALLING edge)
  Fuel sender (via signal cond.) ─► GPIO1  (PIN_FUEL_SENDER_ADC)
  Battery sense (via divider) ────► GPIO2  (PIN_BATTERY_ADC)
  Charging sense (via divider) ───► GPIO3  (PIN_CHARGE_ADC)
  DS18B20 x2 (OneWire, shared) ───► GPIO15 (PIN_ONEWIRE_BUS, 4.7k pull-up to 3.3V)
  I2C bus (IMU/RTC/BMP280/BH1750) ► GPIO21/22 (SDA/SCL, 4.7k pull-ups)
```

```
Discrete harness inputs (ALL through opto-isolators — see docs/power_distribution.md)
  Left indicator ──► GPIO33   Right indicator ──► GPIO34
  Neutral switch ──► GPIO35   High beam ────────► GPIO36
  Horn switch ─────► GPIO37   Side stand ───────► GPIO38
  Front brake ─────► GPIO39   Rear brake ───────► GPIO40
  Clutch ──────────► GPIO41   Kill switch ──────► GPIO42
  Ignition sense ──► GPIO45   Starter active ───► GPIO46
```

```
Outputs
  DRL PWM ─────────► GPIO47 (through a MOSFET driver stage — GPIO cannot
                              source headlight-level current directly)
  Hazard relay ────► GPIO48 (drives a relay coil via transistor, not direct)
  RGB accent data ─► GPIO26 (WS2812B, own 5V feed + level shifter to 5V logic
                              if your strip needs it — many tolerate 3.3V data)
  Buzzer ──────────► GPIO27 (through a transistor if using a large piezo)
```

```
Display / storage / GPS
  TFT: CS/DC/RST ──► GPIO10/11/12   Backlight PWM ──► GPIO9
  Touch: CS/IRQ ───► GPIO13/14
  SD card (shared SPI bus, separate CS) ──► GPIO5 (CS), GPIO38/39/40 (MOSI/MISO/SCK)*
  GPS UART ────────► GPIO43 (TX from ESP32) / GPIO44 (RX to ESP32)
```
\* Note: `Config.h` currently has PIN_SD_MOSI/MISO/SCK sharing numbers with
some discrete inputs (38/39/40) as placeholders — **resolve this pin
conflict for your specific board** before wiring. ESP32-S3 has enough GPIO
to give SD its own dedicated SPI pins; treat the numbers in `Config.h` as a
starting template, not a final pinout, and verify with `pio run` pin-conflict
warnings plus a continuity check before powering real hardware.

## Connector recommendations

| Run | Connector | Why |
|---|---|---|
| Main harness tap (12V, ground, ignition-sense) | Deutsch DT-series or JST-VH | Sealed, vibration-rated, automotive-standard |
| Sensor breakout (speed/RPM/fuel/temp) | Molex MX150 or waterproof JST | Keeps sensor wiring serviceable without harness surgery |
| Display/enclosure connector | Board-to-board or short waterproof pigtail | Minimize length inside a sealed enclosure |
| GPS antenna (if external) | SMA, weatherproofed with self-amalgamating tape | Standard for active GPS antennas |

## Routing notes

- Keep the RPM pickup wire short and routed away from the ignition coil's
  primary/secondary leads — even opto-isolated, inductive pickup on a long
  parallel run reintroduces noise before the isolator can filter it.
- Route sensor wiring on the opposite side of the frame from the exhaust
  where possible — DS18B20 probes are rated for engine-bay heat but the
  wire insulation and any nearby connectors are the actual weak point.
- Leave a drip loop on every wire entering the enclosure — water follows a
  wire down into a housing if it's the lowest point in the run; a loop that
  dips below the entry point before rising into the enclosure defeats that.
- Dielectric grease in every connector before final assembly, standard
  practice for anything living outside the fairing.
