# Calibration Procedures

## Wheel circumference (speed accuracy)
1. Confirm `HALL_PULSES_PER_REV` in `include/Config.h` matches how many
   magnets you actually glued around the wheel (4 is the default/recommended
   count — see `docs/bom.md`). This must be right before the distance
   calibration below means anything.
2. Mark the tyre contact point and the ground with chalk.
2. Push the bike forward (engine off) for exactly 10 wheel revolutions,
   counting hall-sensor pulses via serial monitor (`SensorManager` logs
   raw pulse count if `CORE_DEBUG_LEVEL >= 4`).
3. Measure the total distance rolled in meters.
4. `WHEEL_CIRCUMFERENCE_M = distance_m / 10`.
5. Update the constant in `include/Config.h` and re-flash. (A future
   Settings-screen Calibration Wizard should make this a runtime NVS value
   instead of a compile-time constant — see roadmap Phase 2.)

## Fuel sender curve
The stock float sender is rarely linear across the tank. `SensorManager`
currently uses a straight-line 0-3.3V → 0-100% map as a placeholder.
1. With the tank at known fill levels (empty, 25%, 50%, 75%, full — fill
   with a measured fuel can), record `analogReadMilliVolts(PIN_FUEL_SENDER_ADC)`
   at each point.
2. Fit a lookup table (5+ points, linear-interpolated between them) and
   replace the linear map in `SensorManager::readAnalogChannels()`.
3. Re-verify `RideManager`'s self-calibrating km-per-percent range estimate
   converges sensibly over a full tank before trusting the "distance to
   empty" readout.

## IMU crash-detection threshold
The default 2.5g heuristic in `SensorManager::readImu()` **will** false-
trigger on hard braking, potholes, or aggressive riding unless tuned for
the specific bike + mounting.
1. Log raw `accelMagnitude` over a variety of real riding conditions (hard
   braking, speed bumps, potholes, normal riding) to the SD card.
2. Review the log to find the actual separation point between "hard
   riding" and "this bike is now on the ground."
3. Consider requiring a *sustained* high-g reading (e.g. 100ms) plus a
   post-event check (no wheel-speed pulses for N seconds afterward) before
   raising `CRASH_DETECTED` — a single-sample threshold alone is not
   reliable enough to trigger an emergency SOS call. This is flagged as a
   Phase 3/4 item, not something to ship on threshold-alone.

## IMU zero-point (lean angle)
Mount the sensor, sit the bike perfectly vertical on level ground (use a
level, not the side stand — side-stand lean will bias your zero point),
and record the accelerometer's roll reading as the zero offset to subtract
in `SensorManager::readImu()`.
