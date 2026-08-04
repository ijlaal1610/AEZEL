# Power Distribution — AEZEL

```
Battery (+12V nominal, 9-16V realistic swing, load-dump spikes to 40V+)
   │
   ├── Inline 3A automotive blade fuse
   │
   ├── Reverse-polarity Schottky diode (SS54)
   │
   ├── TVS diode (SMBJ18A, clamps >18V transients to ground)
   │
   ├── Automotive buck converter (wide input 6-40V → stable 5V, load-dump rated)
   │        │
   │        ├── 5V rail → ESP32-S3 module (via onboard 3.3V LDO)
   │        ├── 5V rail → TFT display + backlight (PWM-dimmed)
   │        ├── 5V rail → GPS module
   │        ├── 5V rail → WS2812B accent strip (own 100-220µF bulk cap at strip input)
   │        └── 5V rail → SD card (via level-shifted 3.3V, most modules regulate onboard)
   │
   └── Ignition-switched 12V line (separate harness tap, NOT board power)
            │
            └── Opto-isolator → PIN_IGNITION_SENSE (GPIO logic only)
```

## Key design rules

1. **Board power is always-on from battery-positive**, gated only by the
   buck converter — never by the ignition switch. Ignition is read as a
   *logic signal*, not a power source. This is what makes safe-shutdown
   (flush NVS/SD, play goodbye animation, then deep-sleep) possible instead
   of an abrupt board power loss on every key-off.
2. Every discrete 12V input (indicators, brake switches, kill switch,
   starter-active, ignition-sense) passes through a **PC817 opto-isolator**
   with an input-side current-limiting resistor sized for the motorcycle's
   actual 12V rail, and a pull-up on the output (ESP32) side. This
   physically isolates the ESP32's 3.3V logic domain from the motorcycle's
   noisy, high-voltage electrical system.
3. Put a **100nF ceramic + 10µF bulk capacitor** at the input of every
   downstream module (display, GPS, ESP32) — motorcycle electrical systems
   are electrically noisy (ignition coil switching, starter motor inrush)
   and local decoupling matters far more here than on a bench project.
4. **Deep-sleep current target: <2mA.** With ESP32-S3 deep sleep (~7-10µA)
   plus the buck converter's own quiescent draw (~1-2mA on a good automotive
   module), this keeps a multi-week parked bike from meaningfully draining
   a healthy battery. Verify your specific buck converter's no-load
   quiescent current — cheap modules can be 10-20mA idle, which is *not*
   acceptable for always-on automotive use.
5. Size the fuse to protect the wiring harness, not the electronics — 3A on
   the main feed is generous headroom above the board's actual ~500mA-1A
   peak draw (display backlight + WiFi/BLE radio burst is the worst case).
