# Testing Checklist

## Bench (before any install on the bike)
- [ ] Power on with bench supply at 9V, 12V, 14V, 16V — confirm stable boot
      at every point in the real operating range
- [ ] Simulate a load-dump spike (if you have a surge generator) — confirm
      TVS diode clamps it and the board survives
- [ ] Reverse the battery leads deliberately (fused, current-limited bench
      supply only!) — confirm the reverse-polarity diode protects the board
- [ ] Toggle ignition-sense input rapidly — confirm no false wake/sleep
      cycling (debounce if needed)
- [ ] Pull power abruptly mid-write to NVS/SD — confirm no corruption on
      next boot (repeat 20+ times; this is the failure mode that actually
      happens in the field)
- [ ] Confirm deep-sleep current draw with a multimeter in series — must be
      under the target in `docs/power_distribution.md`
- [ ] Watchdog test: intentionally block a task (e.g. `while(1);` in a test
      build) — confirm the system resets rather than hanging silently

## Installed, engine off
- [ ] All discrete inputs (indicators, brakes, kill switch, neutral,
      high-beam) read correctly with the actual harness, engine off,
      ignition on
- [ ] GPS achieves a fix in reasonable time in open sky
- [ ] BLE companion app connects and receives telemetry
- [ ] SD card read/write confirmed stable after being vibration-mounted

## Installed, engine running (stationary, on stand)
- [ ] RPM reading matches a known-good tachometer (handheld or reference)
      across idle → redline
- [ ] No RPM/speed noise spikes from ignition coil switching (scope the
      opto-isolator output if anything looks off)
- [ ] Engine temp sensor reads sensibly as the engine warms up
- [ ] Charging voltage reads correctly once revs are up (alternator online)

## Road test (start short, low-speed, well before trusting any warning logic)
- [ ] Speed reading cross-checked against GPS speed and, ideally, a phone
      GPS speedometer app, across a range of speeds
- [ ] Trip/odometer accumulate correctly compared to a known route distance
- [ ] Warnings behave correctly (don't false-trigger fuel-low, battery-low,
      or crash-detected under normal riding — see `docs/calibration.md`)
- [ ] Confirm ignition-off → linger → safe shutdown → deep sleep → wake on
      next ignition-on all work correctly, repeatedly, including a "quick
      restart" case (ignition off then immediately back on)
- [ ] Long-duration test (multi-hour ride) for thermal stability of the
      display/enclosure and for any memory-leak-driven slowdown
      (`freeHeapBytes` in diagnostics should stay flat over hours)

## Never skip before calling a warning feature "done"
- [ ] False-positive rate over real riding conditions is acceptably low —
      an alarm that cries wolf gets ignored, which defeats the purpose of
      having it
