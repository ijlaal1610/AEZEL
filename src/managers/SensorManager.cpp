#include "SensorManager.h"
#include "Config.h"
#include "VehicleMath.h"
#include <Wire.h>
#if ENABLE_ONEWIRE_TEMP
#include <OneWire.h>
#include <DallasTemperature.h>
#endif
#if ENABLE_IMU
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#endif
#if ENABLE_BAROMETER
#include <Adafruit_BMP280.h>
#endif

volatile uint32_t SensorManager::_speedPulseCount = 0;
volatile uint32_t SensorManager::_rpmPulseCount = 0;

#if ENABLE_ONEWIRE_TEMP
static OneWire oneWire(PIN_ONEWIRE_BUS);
static DallasTemperature dallas(&oneWire);
#endif
#if ENABLE_IMU
static Adafruit_MPU6050 mpu;
#endif
#if ENABLE_BAROMETER
static Adafruit_BMP280 bmp;
#endif

void IRAM_ATTR SensorManager::isrSpeedPulse() { _speedPulseCount++; }
void IRAM_ATTR SensorManager::isrRpmPulse()   { _rpmPulseCount++; }

void SensorManager::begin() {
    pinMode(PIN_SPEED_HALL, INPUT_PULLUP);
    pinMode(PIN_RPM_PICKUP, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_SPEED_HALL), isrSpeedPulse, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_RPM_PICKUP), isrRpmPulse, FALLING);

    // Indicator / discrete inputs — all opto-isolated, so INPUT (no pull needed,
    // isolator output stage defines the level). Adjust if your isolator board
    // is open-drain (then use INPUT_PULLUP).
    const int discretePins[] = {
        PIN_IN_LEFT_INDICATOR, PIN_IN_RIGHT_INDICATOR, PIN_IN_NEUTRAL, PIN_IN_HIGH_BEAM,
        PIN_IN_SIDE_STAND, PIN_IN_FRONT_BRAKE, PIN_IN_REAR_BRAKE, PIN_IN_CLUTCH,
        PIN_IN_KILL_SWITCH, PIN_IN_IGNITION, PIN_IN_STARTER, PIN_IN_HORN_SW
    };
    for (int p : discretePins) pinMode(p, INPUT);

    analogReadResolution(12);   // 0-4095
    Wire.begin(I2C_SDA, I2C_SCL);

#if ENABLE_ONEWIRE_TEMP
    dallas.begin();
#endif
#if ENABLE_IMU
    _imuOk = mpu.begin();
    if (_imuOk) {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }
#endif
#if ENABLE_BAROMETER
    _envOk = bmp.begin(0x76);
#endif

    _lastTickMs = millis();
    _lastSpeedRpmCalcMs = _lastTickMs;
    // Every #if above defaults OFF in Config.h until you install that part —
    // this manager runs fine with only the hall/RPM sensors wired, which is
    // all Tier 0/1 requires. See docs/incremental_build.md.
}

void SensorManager::taskEntry(void* pv) {
    SensorManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(50);   // 20 Hz
    TickType_t lastWake = xTaskGetTickCount();
    for (;;) {
        self.tick();
        vTaskDelayUntil(&lastWake, period);
    }
}

void SensorManager::tick() {
    uint32_t now = millis();
    float dt = (now - _lastTickMs) / 1000.0f;
    if (dt <= 0) dt = 0.05f;
    _lastTickMs = now;

    // Speed/RPM: accumulate pulses over SPEED_RPM_CALC_WINDOW_MS rather than
    // every 50ms tick — see the member comment in SensorManager.h for why a
    // narrower window quantizes badly with a low-pulse-count wheel sensor.
    // The pulse counters themselves are still updated by ISR continuously;
    // we're only changing how often we READ and RESET them.
    uint32_t sinceSpeedCalc = now - _lastSpeedRpmCalcMs;
    if (sinceSpeedCalc >= SPEED_RPM_CALC_WINDOW_MS) {
        float speedDt = sinceSpeedCalc / 1000.0f;
        computeSpeed(speedDt);
        computeRpm(speedDt);
        _lastSpeedRpmCalcMs = now;
    }

    readAnalogChannels();
    updateIndicatorInputs();

    // Slower-rate sensors don't need every 50ms tick — stagger them.
    static uint8_t divider = 0;
    divider++;
    if (divider % 4 == 0) readOneWireTemps();     // ~5 Hz
    if (divider % 2 == 0) readImu();              // ~10 Hz
    if (divider % 20 == 0) readEnvironmental();   // 1 Hz

    evaluateWarnings();
}

void SensorManager::computeSpeed(float dtSec) {
    noInterrupts();
    uint32_t pulses = _speedPulseCount;
    _speedPulseCount = 0;
    interrupts();

    float instSpeedKmh = VehicleMath::pulsesToKmh(pulses, HALL_PULSES_PER_REV,
                                                    WHEEL_CIRCUMFERENCE_M, dtSec);

    // EMA smoothing (alpha=0.35) — responsive but not needle-jittery
    _speedEma = VehicleMath::emaUpdate(_speedEma, instSpeedKmh, 0.35f);
    if (_speedEma < 0.15f) _speedEma = 0;   // stopped

    SharedState::instance().update([&](VehicleState& s) {
        s.speedKmh = _speedEma;
        if (_speedEma > s.maxSpeedKmh) s.maxSpeedKmh = _speedEma;
        // Distance accumulation happens in RideManager (owns trip/odo persistence)
    });
}

void SensorManager::computeRpm(float dtSec) {
    noInterrupts();
    uint32_t pulses = _rpmPulseCount;
    _rpmPulseCount = 0;
    interrupts();

    float rpm = VehicleMath::pulsesToRpm(pulses, RPM_PICKUP_PULSES_PER_REV, dtSec);
    _rpmEma = VehicleMath::emaUpdate(_rpmEma, rpm, 0.4f);

    SharedState::instance().update([&](VehicleState& s) {
        s.rpm = (uint16_t)_rpmEma;
        s.inEngineRunning = _rpmEma > 300;   // idle threshold
    });
}

void SensorManager::readAnalogChannels() {
    // Battery / charging voltage via dividers
    float vbat = VehicleMath::dividerVoltage(analogReadMilliVolts(PIN_BATTERY_ADC), BATTERY_DIVIDER_RATIO);
    float vchg = VehicleMath::dividerVoltage(analogReadMilliVolts(PIN_CHARGE_ADC), CHARGE_DIVIDER_RATIO);

#if ENABLE_FUEL_SENDER
    // Fuel sender: resistive float, conditioned to 0-3.3V by divider on the
    // hardware board. Replace this linear map with your tank's calibration
    // curve captured during the Calibration Wizard (see docs/calibration.md).
    float fuelRaw = analogReadMilliVolts(PIN_FUEL_SENDER_ADC) / 1000.0f;
    float fuelPct = constrain((fuelRaw / 3.3f) * 100.0f, 0.0f, 100.0f);
    _fuelEma = VehicleMath::emaUpdate(_fuelEma, fuelPct, 0.05f);   // heavy smoothing (fuel sloshes)
#endif

    SharedState::instance().update([&](VehicleState& s) {
        s.batteryVoltage = vbat;
        s.chargingVoltage = vchg;
#if ENABLE_FUEL_SENDER
        s.fuelLevelPct = _fuelEma;
#endif
        // Range/consumption computed in RideManager where trip distance lives
    });
}

void SensorManager::readOneWireTemps() {
#if ENABLE_ONEWIRE_TEMP
    dallas.requestTemperatures();
    float engineC = dallas.getTempCByIndex(0);
    float ambientC = dallas.getTempCByIndex(1);
    if (engineC == DEVICE_DISCONNECTED_C) return;

    SharedState::instance().update([&](VehicleState& s) {
        s.engineTempC = engineC;
        if (ambientC != DEVICE_DISCONNECTED_C) s.outsideTempC = ambientC;
    });
#endif
}

void SensorManager::readImu() {
#if ENABLE_IMU
    if (!_imuOk) return;
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Lean angle approximated from accelerometer roll — good enough for a
    // dashboard indicator; a fused complementary/Kalman filter (see
    // docs/roadmap.md Phase 3) is recommended before using this for
    // anything safety-critical like cornering-light control.
    float roll = atan2(a.acceleration.y, a.acceleration.z) * 180.0f / PI;
    float pitch = atan2(-a.acceleration.x,
                         sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z))
                  * 180.0f / PI;

    float accelMagnitude = sqrt(a.acceleration.x * a.acceleration.x +
                                 a.acceleration.y * a.acceleration.y +
                                 a.acceleration.z * a.acceleration.z);

    // Crude crash heuristic: sustained high-g outside normal riding range.
    // Tune thresholds per docs/calibration.md before relying on this for SOS.
    bool crash = accelMagnitude > 25.0f;   // ~2.5g spike

    SharedState::instance().update([&](VehicleState& s) {
        s.leanAngleDeg = roll;
        s.pitchDeg = pitch;
        s.crashSuspected = crash;
    });
#endif
}

void SensorManager::readEnvironmental() {
#if ENABLE_BAROMETER
    if (!_envOk) return;
    float pressure = bmp.readPressure() / 100.0f;   // hPa
    float altitude = bmp.readAltitude(1013.25f);     // adjust sea-level ref via Settings

    SharedState::instance().update([&](VehicleState& s) {
        s.pressureHPa = pressure;
        s.altitudeM = altitude;
    });
#endif
}

void SensorManager::updateIndicatorInputs() {
    SharedState::instance().update([&](VehicleState& s) {
        s.inLeftIndicator  = digitalRead(PIN_IN_LEFT_INDICATOR);
        s.inRightIndicator = digitalRead(PIN_IN_RIGHT_INDICATOR);
        s.inHazard         = s.inLeftIndicator && s.inRightIndicator;
        s.inNeutral        = digitalRead(PIN_IN_NEUTRAL);
        s.gear             = s.inNeutral ? GearState::NEUTRAL : s.gear; // real gear position needs a gear-position sensor (optional add-on)
        s.inHighBeam       = digitalRead(PIN_IN_HIGH_BEAM);
        s.inLowBeam         = !s.inHighBeam;
        s.inSideStand       = digitalRead(PIN_IN_SIDE_STAND);
        s.inFrontBrake      = digitalRead(PIN_IN_FRONT_BRAKE);
        s.inRearBrake       = digitalRead(PIN_IN_REAR_BRAKE);
        s.inClutch          = digitalRead(PIN_IN_CLUTCH);
        s.inKillSwitch       = digitalRead(PIN_IN_KILL_SWITCH);
        s.inIgnitionOn        = digitalRead(PIN_IN_IGNITION);
        s.inStarterActive      = digitalRead(PIN_IN_STARTER);
    });
}

void SensorManager::evaluateWarnings() {
    auto& ss = SharedState::instance();
    VehicleState s = ss.snapshot();

    (s.batteryVoltage > 0 && s.batteryVoltage < 11.8f && s.inEngineRunning)
        ? ss.raiseWarning(WarningFlag::BATTERY_LOW) : ss.clearWarning(WarningFlag::BATTERY_LOW);

    (s.inEngineRunning && s.chargingVoltage < 13.0f)
        ? ss.raiseWarning(WarningFlag::CHARGING_FAULT) : ss.clearWarning(WarningFlag::CHARGING_FAULT);

    (s.engineTempC > 110.0f)
        ? ss.raiseWarning(WarningFlag::ENGINE_OVERTEMP) : ss.clearWarning(WarningFlag::ENGINE_OVERTEMP);

#if ENABLE_FUEL_SENDER
    (s.fuelLevelPct < 15.0f)
        ? ss.raiseWarning(WarningFlag::FUEL_LOW) : ss.clearWarning(WarningFlag::FUEL_LOW);
#endif

    if (s.crashSuspected) ss.raiseWarning(WarningFlag::CRASH_DETECTED);
}
