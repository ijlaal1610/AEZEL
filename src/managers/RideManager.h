#pragma once
// ============================================================================
//  RideManager — integrates speed into distance, owns trip/odometer state,
//  ride timer, average/max speed, fuel range & consumption, and periodic
//  ride-log points written through StorageManager.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

class RideManager {
public:
    static RideManager& instance() { static RideManager r; return r; }

    void begin();                     // loads odometer/trip from NVS
    static void taskEntry(void* pv);
    void tick();

    void resetTripA();
    void resetTripB();

private:
    RideManager() = default;
    void integrateDistance(float dtSec);
    void updateFuelEstimate();
    void updateRideTimerAndAverages(float dtSec);
    void maybeLogPoint();

    float _odometerKm = 0, _tripAKm = 0, _tripBKm = 0;
    float _distanceAccumM = 0;         // sub-metre accumulator to avoid float precision loss
    float _speedSumForAvg = 0;
    uint32_t _avgSampleCount = 0;
    uint32_t _rideTimerSec = 0;
    uint32_t _lastTickMs = 0;
    uint32_t _lastLogMs = 0;

    // Fuel-consumption model: distance covered per % fuel used, smoothed.
    float _fuelPctAtLastCheckpoint = -1;
    float _kmAtLastCheckpoint = 0;
    float _kmPerPercent = 0.35f;    // seeded estimate; refines as the tank empties

    static constexpr float TANK_CAPACITY_L = 9.0f;   // Avenger 150 stock tank
    static constexpr uint32_t LOG_INTERVAL_MS = 3000; // one GPS/ride point every 3s
};
