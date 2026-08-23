#pragma once
// ============================================================================
//  DisplayPolicyMath.h — pure decisions about what the dashboard shows and
//  when, with zero hardware dependency (same philosophy as VehicleMath.h).
//  Two small but easy-to-get-backwards questions live here:
//    - should a banner be suppressed right now (quiet mode)?
//    - is this phone-pushed data (call/music/nav) too old to trust?
//  Getting either wrong is a real UX bug (an alarm silently suppressed at
//  speed, or a stale "turn right" arrow lingering after the phone
//  disconnected), so both are worth pinning down with tests before they're
//  wired into DisplayManager.
// ============================================================================
#include <cstdint>

namespace DisplayPolicyMath {

// CRITICAL warnings are NEVER suppressed regardless of speed — quiet mode
// is about reducing non-critical clutter while riding, not about hiding
// something that matters. Pass isCritical=true and this always returns
// false.
bool shouldSuppressBanner(bool isCritical, float speedKmh, float quietModeThresholdKmh);

// True if data last updated at lastUpdateMs is still fresh at nowMs, given
// a staleness window. lastUpdateMs == 0 is the "never received any data"
// sentinel and is always stale (never fresh), regardless of nowMs.
bool isDataFresh(uint32_t lastUpdateMs, uint32_t nowMs, uint32_t staleAfterMs);

}  // namespace DisplayPolicyMath
