#pragma once
// ============================================================================
//  MaintenanceMath.h — pure "is this due yet" logic for odometer- and
//  date-based maintenance items (service, tyres, chain, insurance, PUC).
//  Zero hardware dependency, same philosophy as VehicleMath.h — the
//  off-by-one/sign mistakes in "is X due" logic are exactly the kind of
//  thing worth catching on a desktop compiler before they either nag the
//  rider constantly or silently never fire.
// ============================================================================
#include <cstdint>

enum class MaintenanceStatus : uint8_t { OK, UPCOMING, OVERDUE };

namespace MaintenanceMath {

// currentKm/dueKm in kilometers, warnWindowKm = how far before the due
// point to start showing UPCOMING instead of OK. A dueKm of 0 means "not
// configured" and always returns OK — callers should treat 0 as "don't
// evaluate this item" rather than "due at km 0."
MaintenanceStatus distanceStatus(float currentKm, uint32_t dueKm, float warnWindowKm);

// nowEpochSec/dueEpochSec in Unix seconds, warnWindowSec = how long before
// the due date to start showing UPCOMING. A dueEpochSec of 0 means "not
// configured" and always returns OK, same convention as distanceStatus.
MaintenanceStatus dateStatus(uint32_t nowEpochSec, uint32_t dueEpochSec, uint32_t warnWindowSec);

// Computes the next due odometer reading after resetting a distance-based
// item (e.g. "mark serviced") — current position plus the interval.
uint32_t nextDueOdometer(float currentKm, uint32_t intervalKm);

}  // namespace MaintenanceMath
