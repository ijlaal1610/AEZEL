#include "RideManager.h"
#include "StorageManager.h"
#include "VehicleMath.h"
#include <time.h>

void RideManager::begin() {
    auto& storage = StorageManager::instance();
    _odometerKm = storage.loadOdometer();
    _tripAKm = storage.loadTripA();
    _tripBKm = storage.loadTripB();
    _lastTickMs = millis();

    SharedState::instance().update([&](VehicleState& s) {
        s.odometer_km = _odometerKm;
        s.tripA_km = _tripAKm;
        s.tripB_km = _tripBKm;
    });
}

void RideManager::taskEntry(void* pv) {
    RideManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(200);   // 5 Hz — distance integration doesn't need more
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void RideManager::tick() {
    uint32_t now = millis();
    float dt = (now - _lastTickMs) / 1000.0f;
    if (dt <= 0) dt = 0.2f;
    _lastTickMs = now;

    VehicleState s = SharedState::instance().snapshot();

    // Only integrate distance/time while engine is running — avoids trip
    // creeping while parked with ignition on.
    if (s.inEngineRunning) {
        integrateDistance(dt);
        updateRideTimerAndAverages(dt);
        updateFuelEstimate();
        maybeLogPoint();
    }
}

void RideManager::integrateDistance(float dtSec) {
    VehicleState s = SharedState::instance().snapshot();
    float meters = VehicleMath::speedToMeters(s.speedKmh, dtSec);
    _distanceAccumM += meters;

    if (_distanceAccumM >= 1.0f) {
        float km = _distanceAccumM / 1000.0f;
        _distanceAccumM = fmodf(_distanceAccumM, 1000.0f);
        _odometerKm += km;
        _tripAKm += km;
        _tripBKm += km;

        SharedState::instance().update([&](VehicleState& st) {
            st.odometer_km = _odometerKm;
            st.tripA_km = _tripAKm;
            st.tripB_km = _tripBKm;
        });

        // Persisted at low frequency by StorageManager's own flush timer —
        // we just hand it the latest value each time it changes meaningfully.
        StorageManager::instance().saveOdometer(_odometerKm);
        StorageManager::instance().saveTripA(_tripAKm);
        StorageManager::instance().saveTripB(_tripBKm);
    }
}

void RideManager::updateRideTimerAndAverages(float dtSec) {
    _rideTimerSec += (uint32_t)dtSec;   // coarse but fine at 5Hz tick with dt~0.2s accumulation
    static float subSecAccum = 0;
    subSecAccum += dtSec;
    if (subSecAccum >= 1.0f) { _rideTimerSec += (uint32_t)subSecAccum; subSecAccum = fmodf(subSecAccum, 1.0f); }

    VehicleState s = SharedState::instance().snapshot();
    _speedSumForAvg += s.speedKmh;
    _avgSampleCount++;
    float avg = _avgSampleCount ? (_speedSumForAvg / _avgSampleCount) : 0;

    SharedState::instance().update([&](VehicleState& st) {
        st.rideTimerSec = _rideTimerSec;
        st.avgSpeedKmh = avg;
    });
}

void RideManager::updateFuelEstimate() {
    VehicleState s = SharedState::instance().snapshot();

    if (_fuelPctAtLastCheckpoint < 0) {
        _fuelPctAtLastCheckpoint = s.fuelLevelPct;
        _kmAtLastCheckpoint = _odometerKm;
    }

    float fuelDropPct = _fuelPctAtLastCheckpoint - s.fuelLevelPct;
    // Recalibrate km-per-percent once a meaningful drop is observed —
    // avoids using stale seed value forever, converges as tank empties.
    if (fuelDropPct >= 2.0f) {
        float kmCovered = _odometerKm - _kmAtLastCheckpoint;
        _kmPerPercent = VehicleMath::recalibrateKmPerPercent(_kmPerPercent, kmCovered, fuelDropPct, 0.3f);
        _fuelPctAtLastCheckpoint = s.fuelLevelPct;
        _kmAtLastCheckpoint = _odometerKm;
    }

    float rangeKm = VehicleMath::fuelRangeKm(s.fuelLevelPct, _kmPerPercent);
    float kmPerLiterVal = VehicleMath::kmPerLiter(_kmPerPercent, TANK_CAPACITY_L);

    SharedState::instance().update([&](VehicleState& st) {
        st.fuelRangeKm = rangeKm;
        st.fuelConsumptionKmL = kmPerLiterVal;
    });
}

void RideManager::maybeLogPoint() {
    uint32_t now = millis();
    if (now - _lastLogMs < LOG_INTERVAL_MS) return;
    _lastLogMs = now;

    VehicleState s = SharedState::instance().snapshot();
    if (!s.gpsFixValid) return;   // no point logging without position

    RideLogPoint pt{};
    pt.timestamp = time(nullptr);
    pt.lat = s.latitude;
    pt.lon = s.longitude;
    pt.speedKmh = s.speedKmh;
    pt.rpm = s.rpm;
    pt.leanAngle = s.leanAngleDeg;
    StorageManager::instance().logRidePoint(pt);
}

void RideManager::resetTripA() {
    _tripAKm = 0;
    StorageManager::instance().saveTripA(0);
    SharedState::instance().update([](VehicleState& s) { s.tripA_km = 0; });
}
void RideManager::resetTripB() {
    _tripBKm = 0;
    StorageManager::instance().saveTripB(0);
    SharedState::instance().update([](VehicleState& s) { s.tripB_km = 0; });
}
