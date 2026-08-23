#include "SecurityMath.h"

namespace SecurityMath {

float gpsDriftMeters(double baseLat, double baseLon, double curLat, double curLon) {
    constexpr double kEarthRadiusM = 6371000.0;
    constexpr double kDegToRad = 3.14159265358979323846 / 180.0;

    double dLat = (curLat - baseLat) * kDegToRad;
    double dLon = (curLon - baseLon) * kDegToRad;
    double meanLat = ((baseLat + curLat) / 2.0) * kDegToRad;

    double x = dLon * std::cos(meanLat);
    double y = dLat;
    return (float)(std::sqrt(x * x + y * y) * kEarthRadiusM);
}

bool angleDeviationExceeds(float baselineDeg, float currentDeg, float thresholdDeg) {
    return std::fabs(currentDeg - baselineDeg) > thresholdDeg;
}

}  // namespace SecurityMath
