#include "MaintenanceMath.h"

namespace MaintenanceMath {

MaintenanceStatus distanceStatus(float currentKm, uint32_t dueKm, float warnWindowKm) {
    if (dueKm == 0) return MaintenanceStatus::OK;   // not configured
    if (currentKm >= (float)dueKm) return MaintenanceStatus::OVERDUE;
    if ((float)dueKm - currentKm <= warnWindowKm) return MaintenanceStatus::UPCOMING;
    return MaintenanceStatus::OK;
}

MaintenanceStatus dateStatus(uint32_t nowEpochSec, uint32_t dueEpochSec, uint32_t warnWindowSec) {
    if (dueEpochSec == 0) return MaintenanceStatus::OK;   // not configured
    if (nowEpochSec >= dueEpochSec) return MaintenanceStatus::OVERDUE;
    if (dueEpochSec - nowEpochSec <= warnWindowSec) return MaintenanceStatus::UPCOMING;
    return MaintenanceStatus::OK;
}

uint32_t nextDueOdometer(float currentKm, uint32_t intervalKm) {
    return (uint32_t)currentKm + intervalKm;
}

}  // namespace MaintenanceMath
