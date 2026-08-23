#include "RideModeProfile.h"

namespace RideModeProfiles {

// clang-format off
static const RideModeProfile kEco = {
    ThemeMode::LIGHT,   150,   20.0f,  110.0f,  8000
    // Eco: dimmer display (saves a little power), warns about fuel a bit
    // earlier (20% not 15%) since the point of the mode is planning ahead
    // for efficiency, and logs less often — fewer SD writes on a mode
    // that's about restraint, not spirited data-gathering.
};
static const RideModeProfile kCity = {
    ThemeMode::MODERN_DIGITAL,  200,  15.0f,  110.0f,  3000
    // City: the default/baseline profile every other mode is described
    // relative to.
};
static const RideModeProfile kTouring = {
    ThemeMode::CLASSIC_ANALOG,  220,  15.0f,  110.0f,  2000
    // Touring: brighter (long days, varied light), logs more frequently —
    // a multi-hour trip is exactly the ride you want a detailed GPX track
    // for afterward.
};
static const RideModeProfile kSport = {
    ThemeMode::SPORT,  255,  15.0f,  100.0f,  1000
    // Sport: max brightness for visibility at speed, a TIGHTER overtemp
    // threshold (100C not 110C) since spirited riding heats an
    // air-cooled single faster and you want the warning sooner, and the
    // highest logging rate for the best post-ride analytics.
};
static const RideModeProfile kRain = {
    ThemeMode::DARK,  180,  15.0f,  108.0f,  3000
    // Rain: darker theme reduces glare/reflection on a wet visor, slightly
    // more conservative overtemp margin. DisplayManager also pushes a
    // one-time "reduced traction" advisory on switching INTO this mode
    // (see DisplayManager::applyRideModeProfile) — there's no actual
    // traction control on this build, so the mode is an attention cue,
    // not a system that changes how the bike behaves.
};
static const RideModeProfile kCustom = {
    ThemeMode::CUSTOM,  255,  15.0f,  110.0f,  3000
    // Custom: no brightness cap (255) so the user's manual slider choice
    // in Settings is fully respected rather than fighting a mode default —
    // this is the "I want to set everything myself" mode.
};
// clang-format on

const RideModeProfile& get(RideMode mode) {
    switch (mode) {
        case RideMode::ECO:     return kEco;
        case RideMode::CITY:    return kCity;
        case RideMode::TOURING: return kTouring;
        case RideMode::SPORT:   return kSport;
        case RideMode::RAIN:    return kRain;
        case RideMode::CUSTOM:  return kCustom;
        default:                return kCity;   // unknown mode -> safe default, never garbage
    }
}

}  // namespace RideModeProfiles
