#pragma once
// ============================================================================
//  MaintenanceManager — the missing link between the SERVICE_DUE/TYRE_DUE/
//  CHAIN_LUBE_DUE/INSURANCE_EXPIRING/PUC_EXPIRING warning flags (which
//  existed in VehicleEnums.h) and NotificationManager's titles for them
//  (which existed in NotificationManager.cpp) — nothing ever actually
//  raised these flags until this manager. See docs/maintenance.md.
//
//  Service and chain-lube are odometer-based and self-schedule: the first
//  time this manager runs with no stored due value, it sets one
//  (current odometer + interval) rather than firing a warning immediately
//  on a freshly-flashed board. Tyre wear (odometer-based) and insurance/
//  PUC expiry (date-based) can't be guessed from firmware — they stay
//  unconfigured (never warn) until set via a BLE command, see BleManager.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

class MaintenanceManager {
public:
    static MaintenanceManager& instance() { static MaintenanceManager m; return m; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    // Called from the Settings screen's "Mark Serviced" / "Mark Chain
    // Lubed" buttons — resets that item's due point to now + interval.
    void markServiced();
    void markChainLubed();

    // Called from BleManager's command handler when the companion app
    // configures an item firmware can't guess a default for.
    void setTyreDueKm(uint32_t dueKm);
    void setInsuranceDueTs(uint32_t dueEpochSec);
    void setPucDueTs(uint32_t dueEpochSec);

    // Read access for the Settings screen's status labels.
    uint32_t serviceDueKm() const { return _serviceDueKm; }
    uint32_t chainDueKm() const { return _chainDueKm; }

private:
    MaintenanceManager() = default;
    void evaluateAll();

    uint32_t _serviceDueKm = 0, _chainDueKm = 0, _tyreDueKm = 0;
    uint32_t _insuranceDueTs = 0, _pucDueTs = 0;
};
