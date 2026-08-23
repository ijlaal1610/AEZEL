#include "DisplayPolicyMath.h"

namespace DisplayPolicyMath {

bool shouldSuppressBanner(bool isCritical, float speedKmh, float quietModeThresholdKmh) {
    if (isCritical) return false;   // never suppress — see header comment
    return speedKmh > quietModeThresholdKmh;
}

bool isDataFresh(uint32_t lastUpdateMs, uint32_t nowMs, uint32_t staleAfterMs) {
    if (lastUpdateMs == 0) return false;         // never received data
    if (nowMs < lastUpdateMs) return false;      // clock rolled over/reset — treat as stale, don't underflow
    return (nowMs - lastUpdateMs) <= staleAfterMs;
}

}  // namespace DisplayPolicyMath
