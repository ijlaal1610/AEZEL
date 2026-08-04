#include "SensorManager.h"
#include "Config.h"
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

volatile uint32_t SensorManager::_speedPulseCount = 0;
volatile uint32_t SensorManager::_rpmPulseCount = 0;

static OneWire oneWire(PIN_ONEWIRE_BUS);
static DallasTemperature dallas(&oneWire);
static Adafruit_MPU6050 mpu;
static Adafruit_BMP280 bmp;

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

    dallas.begin();
    _imuOk = mpu.begin();
    if (_imuOk) {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }
    _envOk = bmp.begin(0x76);

    _lastTickMs = millis();
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

    computeSpeed(dt);
    computeRpm(dt);
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

    float revsPerSec = pulses / (float)HALL_PULSES_PER_REV / dtSec;
    float instSpeedKmh = revsPerSec * WHEEL_CIRCUMFERENCE_M * 3.6f;

    // EMA smoothing (alpha=0.35) — responsive but not needle-jittery
    _speedEma = _speedEma + 0.35f * (instSpeedKmh - _speedEma);
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

    float rpm = (pulses / (float)RPM_PICKUP_PULSES_PER_REV / dtSec) * 60.0f;
    _rpmEma = _rpmEma + 0.4f * (rpm - _rpmEma);

    SharedState::instance().update([&](VehicleState& s) {
        s.rpm = (uint16_t)_rpmEma;
        s.inEngineRunning = _rpmEma > 300;   // idle threshold
    });
}

void SensorManager::readAnalogChannels() {
    // Battery / charging voltage via dividers
    float vbatRaw = analogReadMilliVolts(PIN_BATTERY_ADC) / 1000.0f;
    float vchgRaw = analogReadMilliVolts(PIN_CHARGE_ADC) / 1000.0f;
    float vbat = vbatRaw * BATTERY_DIVIDER_RATIO;
    float vchg = vchgRaw * CHARGE_DIVIDER_RATIO;

    // Fuel sender: resistive float, conditioned to 0-3.3V by divider on the
    // hardware board. Replace this linear map with your tank's calibration
    // curve captured during the Calibration Wizard (see docs/calibration.md).
    float fuelRaw = analogReadMilliVolts(PIN_FUEL_SENDER_ADC) / 1000.0f;
    float fuelPct = constrain((fuelRaw / 3.3f) * 100.0f, 0.0f, 100.0f);
    _fuelEma = _fuelEma + 0.05f * (fuelPct - _fuelEma);   // heavy smoothing (fuel sloshes)

    SharedState::instance().update([&](VehicleState& s) {
        s.batteryVoltage = vbat;
        s.chargingVoltage = vchg;
        s.fuelLevelPct = _fuelEma;
        // Range/consumption computed in RideManager where trip distance lives
    });
}

void SensorManager::readOneWireTemps() {
    dallas.requestTemperatures();
    float engineC = dallas.getTempCByIndex(0);
    float ambientC = dallas.getTempCByIndex(1);
    if (engineC == DEVICE_DISCONNECTED_C) return;

    SharedState::instance().update([&](VehicleState& s) {
        s.engineTempC = engineC;
        if (ambientC != DEVICE_DISCONNECTED_C) s.outsideTempC = ambientC;
    });
}

void SensorManager::readImu() {
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
}

void SensorManager::readEnvironmental() {
    if (!_envOk) return;
    float pressure = bmp.readPressure() / 100.0f;   // hPa
    float altitude = bmp.readAltitude(1013.25f);     // adjust sea-level ref via Settings

    SharedState::instance().update([&](VehicleState& s) {
        s.pressureHPa = pressure;
        s.altitudeM = altitude;
    });
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

    (s.fuelLevelPct < 15.0f)
        ? ss.raiseWarning(WarningFlag::FUEL_LOW) : ss.clearWarning(WarningFlag::FUEL_LOW);

    if (s.crashSuspected) ss.raiseWarning(WarningFlag::CRASH_DETECTED);
}
