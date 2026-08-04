# PCB Design Recommendations — AEZEL

A hand-wired protoboard is fine for Phase 0/1 bench validation
(`docs/roadmap.md`), but a real install should move to a custom PCB before
going on the bike long-term — point-to-point automotive wiring on a
protoboard is exactly the kind of thing that works on the bench and fails
three weeks later from vibration fatigue.

## Recommended board structure

Split into **two boards** connected by a short ribbon/header, not one
monolithic PCB:

1. **Main logic board** — ESP32-S3 module, level-shifters, I2C sensor
   headers, SD socket, GPS header, BLE antenna keep-out zone. Lives inside
   the sealed enclosure, away from the display's heat and away from the
   power board's switching noise.
2. **Power/interface board** — buck converter, reverse-polarity/TVS
   protection, opto-isolator bank for all discrete harness inputs, fuse
   holder, MOSFET drivers for DRL/hazard outputs. Physically separates
   "noisy 12V automotive stuff" from "sensitive 3.3V logic stuff" — even
   with isolation components, board-level separation reduces coupled noise
   further.

## Layout guidance

- Keep the opto-isolator bank's 12V-side traces short and grouped away
  from the ESP32's antenna keep-out area (WiFi/BLE) and away from the
  crystal/oscillator if using an external one.
- Star-ground the power board: battery-negative, buck-converter ground,
  and opto-isolator input-side grounds should NOT share a single trace back
  to the ESP32's ground plane — route each back to a single ground point to
  avoid ground-loop noise on the ADC readings (battery/charging voltage
  sense is especially sensitive to this).
- Put a solid ground pour under the ESP32-S3 module per Espressif's
  reference layout — this is not optional for RF performance (WiFi/BLE
  range) and is a common mistake on hand-drawn hobby boards.
- 2-layer board is adequate for this design; 4-layer only becomes worth the
  cost if you later add CAN bus at high baud rates or run into EMI issues
  during testing that 2-layer routing can't solve.
- Conformal-coat both boards after assembly and testing — this matters more
  than almost any other single decision for long-term reliability in a
  vibrating, humid, temperature-cycling motorcycle environment.

## Recommended tooling

KiCad (free, widely supported by Indian PCB fabs like PCBWay/JLCPCB with
direct Gerber upload) is the natural choice here — no licensing cost is a
real advantage for a solo/small-batch build, and its footprint libraries
already cover the ESP32-S3 module, common connectors, and passive
components used in this BOM.

## Fab & assembly notes

- Order boards in **ENIG finish**, not plain HASL — better long-term
  solderability and corrosion resistance for a board that will see thermal
  cycling and some humidity exposure even inside a sealed enclosure.
- Hand-assemble the through-hole connectors (Deutsch/JST headers,
  fuse holder) yourself even if you get the SMD components
  factory-assembled — connector placement/strain-relief benefits from doing
  it by hand and checking fit against your actual harness before reflow
  commits you to a layout.
