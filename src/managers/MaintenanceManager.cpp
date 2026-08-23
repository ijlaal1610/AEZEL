#include "MaintenanceManager.h"
#include "Config.h"
#include "StorageManager.h"
#include "MaintenanceMath.h"
#include <time.h>

void MaintenanceManager::begin() {
    auto& storage = StorageManager::instance();

    _serviceDueKm = storage.loadMaintenanceKm("svc", 0);
    _chainDueKm = storage.loadMaintenanceKm("chain", 0);
    _tyreDueKm = storage.loadMaintenanceKm("tyre", 0);      // stays 0 (unconfigured) until app sets it
    _insuranceDueTs = storage.loadMaintenanceTs("ins", 0);   // stays 0 (unconfigured) until app sets it
    _pucDueTs = storage.loadMaintenanceTs("puc", 0);

    // Self-schedule service/chain on first-ever boot (stored value of 0
    // means "never set"), so a freshly-flashed board doesn't immediately
    // claim service is overdue at odometer 0.
    float currentOdo = SharedState::instance().snapshot().odometer_km;
    if (_serviceDueKm == 0) {
        _serviceDueKm = MaintenanceMath::nextDueOdometer(currentOdo, SERVICE_INTERVAL_KM);
        storage.saveMaintenanceRecord("svc", _serviceDueKm, 0);
    }
    if (_chainDueKm == 0) {
        _chainDueKm = MaintenanceMath::nextDueOdometer(currentOdo, CHAIN_LUBE_INTERVAL_KM);
        storage.saveMaintenanceRecord("chain", _chainDueKm, 0);
    }
}

void MaintenanceManager::taskEntry(void* pv) {
    MaintenanceManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(MAINTENANCE_CHECK_INTERVAL_MS);
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void MaintenanceManager::tick() { evaluateAll(); }

void MaintenanceManager::evaluateAll() {
    VehicleState s = SharedState::instance().snapshot();
    auto& ss = SharedState::instance();
    uint32_t now = (uint32_t)time(nullptr);

    auto applyDistance = [&](uint32_t dueKm, float warnWindow, WarningFlag flag) {
        MaintenanceStatus status = MaintenanceMath::distanceStatus(s.odometer_km, dueKm, warnWindow);
        (status != MaintenanceStatus::OK) ? ss.raiseWarning(flag) : ss.clearWarning(flag);
    };
    auto applyDate = [&](uint32_t dueTs, uint32_t warnWindow, WarningFlag flag) {
        MaintenanceStatus status = MaintenanceMath::dateStatus(now, dueTs, warnWindow);
        (status != MaintenanceStatus::OK) ? ss.raiseWarning(flag) : ss.clearWarning(flag);
    };

    applyDistance(_serviceDueKm, SERVICE_WARN_WINDOW_KM, WarningFlag::SERVICE_DUE);
    applyDistance(_chainDueKm, CHAIN_WARN_WINDOW_KM, WarningFlag::CHAIN_LUBE_DUE);
    applyDistance(_tyreDueKm, TYRE_WARN_WINDOW_KM, WarningFlag::TYRE_DUE);

    // Date-based checks need a real clock. Without GPS (ENABLE_GPS off, see
    // Config.h) or before the first satellite fix syncs the RTC
    // (GpsManager::syncRtcIfNeeded()), time(nullptr) reads close to zero or
    // whatever the last RTC battery-backed value was — comparing that
    // against a real due date would either falsely claim everything is
    // overdue or falsely clear a real expiry. Skip evaluating (leave
    // whatever state these flags were already in) until the clock is
    // plausibly real.
    constexpr uint32_t PLAUSIBLE_EPOCH_FLOOR = 1700000000UL;   // ~Nov 2023
    if (now > PLAUSIBLE_EPOCH_FLOOR) {
        applyDate(_insuranceDueTs, DATE_WARN_WINDOW_SEC, WarningFlag::INSURANCE_EXPIRING);
        applyDate(_pucDueTs, DATE_WARN_WINDOW_SEC, WarningFlag::PUC_EXPIRING);
    }
}

void MaintenanceManager::markServiced() {
    float currentOdo = SharedState::instance().snapshot().odometer_km;
    _serviceDueKm = MaintenanceMath::nextDueOdometer(currentOdo, SERVICE_INTERVAL_KM);
    StorageManager::instance().saveMaintenanceRecord("svc", _serviceDueKm, 0);
    SharedState::instance().clearWarning(WarningFlag::SERVICE_DUE);
}

void MaintenanceManager::markChainLubed() {
    float currentOdo = SharedState::instance().snapshot().odometer_km;
    _chainDueKm = MaintenanceMath::nextDueOdometer(currentOdo, CHAIN_LUBE_INTERVAL_KM);
    StorageManager::instance().saveMaintenanceRecord("chain", _chainDueKm, 0);
    SharedState::instance().clearWarning(WarningFlag::CHAIN_LUBE_DUE);
}

void MaintenanceManager::setTyreDueKm(uint32_t dueKm) {
    _tyreDueKm = dueKm;
    StorageManager::instance().saveMaintenanceRecord("tyre", dueKm, 0);
}

void MaintenanceManager::setInsuranceDueTs(uint32_t dueEpochSec) {
    _insuranceDueTs = dueEpochSec;
    StorageManager::instance().saveMaintenanceRecord("ins", 0, dueEpochSec);
}

void MaintenanceManager::setPucDueTs(uint32_t dueEpochSec) {
    _pucDueTs = dueEpochSec;
    StorageManager::instance().saveMaintenanceRecord("puc", 0, dueEpochSec);
}
