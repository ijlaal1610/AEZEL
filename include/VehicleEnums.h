#pragma once
// ============================================================================
//  VehicleEnums.h — the enums that describe vehicle/UI state, with ZERO
//  hardware dependency (no Arduino.h, no FreeRTOS). Split out from
//  DataModel.h specifically so pure logic modules (RideModeProfile.h,
//  and anything else that just needs to know what a RideMode IS without
//  needing the mutex-guarded VehicleState machinery around it) can include
//  this alone and stay natively testable — see test/native/.
//
//  DataModel.h includes this and adds the Arduino/FreeRTOS-dependent
//  VehicleState/SharedState machinery on top. Nothing in THIS file should
//  ever gain a hardware dependency — if you need one, it belongs in
//  DataModel.h instead.
// ============================================================================
#include <cstdint>

enum class GearState : uint8_t { NEUTRAL, GEAR_1, GEAR_2, GEAR_3, GEAR_4, GEAR_5, UNKNOWN };
enum class RideMode  : uint8_t { ECO, CITY, TOURING, SPORT, RAIN, CUSTOM };
enum class ThemeMode : uint8_t { LIGHT, DARK, CLASSIC_ANALOG, MODERN_DIGITAL, MINIMAL, SPORT, RETRO, NEON, CYBERPUNK, CUSTOM };

enum class WarningFlag : uint32_t {
    NONE                = 0,
    CHECK_ENGINE        = 1 << 0,
    OIL_PRESSURE        = 1 << 1,
    ENGINE_OVERTEMP     = 1 << 2,
    BATTERY_LOW         = 1 << 3,
    CHARGING_FAULT      = 1 << 4,
    FUEL_LOW            = 1 << 5,
    ABS_FAULT           = 1 << 6,
    SERVICE_DUE         = 1 << 7,
    TYRE_DUE            = 1 << 8,
    CHAIN_LUBE_DUE      = 1 << 9,
    INSURANCE_EXPIRING  = 1 << 10,
    PUC_EXPIRING        = 1 << 11,
    CRASH_DETECTED      = 1 << 12,
    UNAUTHORIZED_MOVE   = 1 << 13,
    GPS_LOST            = 1 << 14,
    SD_CARD_FAULT       = 1 << 15,
};
inline WarningFlag operator|(WarningFlag a, WarningFlag b) { return WarningFlag(uint32_t(a) | uint32_t(b)); }
