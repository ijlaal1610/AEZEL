package com.aezel.companion.data

import org.json.JSONObject

/**
 * Mirrors the JSON fields BleManager::publishTelemetry() actually sends —
 * kept in exact correspondence with the firmware side deliberately, field
 * by field, rather than a "richer" model, so anyone diffing this file
 * against BleManager.cpp can see immediately whether they've drifted.
 * All fields have safe defaults so a partial/malformed packet degrades to
 * "unknown" rather than crashing the parse.
 */
data class VehicleState(
    val speedKmh: Int = 0,
    val rpm: Int = 0,
    val gear: String = "N",
    val fuelPct: Int = 0,
    val fuelRangeKm: Int = 185,
    val batteryVoltage: Float = 0f,
    val engineTempC: Float = 0f,
    val odometerKm: Float = 0f,
    val tripAKm: Float = 0f,
    val tripBKm: Float = 0f,
    val maxSpeedKmh: Int = 0,
    val avgSpeedKmh: Int = 0,
    val leanAngleDeg: Float = 0f,
    val ignitionOn: Boolean = false,
    val showSpeedo: Boolean = true,
    val focusMode: Boolean = false,
    val allowNotifOverlay: Boolean = true,
    val enableLockscreen: Boolean = true,
    val isLocked: Boolean = false,
    val warningFlags: Long = 0L,
    val latitude: Double = 0.0,
    val longitude: Double = 0.0,
    val commandResult: String = "",
    val serviceDueKm: Long = 0L,
    val chainDueKm: Long = 0L,
    val securityState: String = "unknown",
    val phoneAction: String? = null,
    val callActive: Boolean = false,
    val callerName: String? = null,
    val musicTitle: String? = null,
    val musicArtist: String? = null,
    val musicPlaying: Boolean = false,
    val navInstruction: String? = null,
    val navDistanceM: Float? = null,
) {
    /** True if [flag] is currently set in warningFlags — see WarningFlags for the bit definitions. */
    fun hasWarning(flag: Long): Boolean = (warningFlags and flag) != 0L

    companion object {
        fun parseOrNull(json: String): VehicleState? {
            return try {
                val o = JSONObject(json)
                VehicleState(
                    speedKmh = o.optInt("spd", 0),
                    rpm = o.optInt("rpm", 0),
                    gear = o.optString("gear", "N"),
                    fuelPct = o.optInt("fuel", 0),
                    fuelRangeKm = o.optInt("fuel_rng", 185),
                    batteryVoltage = o.optDouble("batt", 0.0).toFloat(),
                    engineTempC = o.optDouble("eng_t", 0.0).toFloat(),
                    odometerKm = o.optDouble("odo", 0.0).toFloat(),
                    tripAKm = o.optDouble("tripA", 0.0).toFloat(),
                    tripBKm = o.optDouble("tripB", 0.0).toFloat(),
                    maxSpeedKmh = o.optInt("max_spd", 0),
                    avgSpeedKmh = o.optInt("avg_spd", 0),
                    leanAngleDeg = o.optDouble("lean", 0.0).toFloat(),
                    ignitionOn = o.optBoolean("ign", false),
                    showSpeedo = o.optBoolean("show_spd", true),
                    focusMode = o.optBoolean("focus", false),
                    allowNotifOverlay = o.optBoolean("notif_ovl", true),
                    enableLockscreen = o.optBoolean("lock_en", true),
                    isLocked = o.optBoolean("locked", false),
                    warningFlags = o.optLong("warn", 0L),
                    latitude = o.optDouble("lat", 0.0),
                    longitude = o.optDouble("lon", 0.0),
                    commandResult = o.optString("cmd_result", ""),
                    serviceDueKm = o.optLong("svc_due_km", 0L),
                    chainDueKm = o.optLong("chain_due_km", 0L),
                    securityState = o.optString("security_state", "unknown"),
                    phoneAction = o.optString("phone_action", "").ifEmpty { null },
                    callActive = o.optBoolean("call_active", false),
                    callerName = o.optString("caller_name", "").ifEmpty { null },
                    musicTitle = o.optString("music_title", "").ifEmpty { null },
                    musicArtist = o.optString("music_artist", "").ifEmpty { null },
                    musicPlaying = o.optBoolean("music_playing", false),
                    navInstruction = o.optString("nav_instruction", "").ifEmpty { null },
                    navDistanceM = if (o.has("nav_distance_m")) o.optDouble("nav_distance_m").toFloat() else null,
                )
            } catch (e: Exception) {
                null
            }
        }
    }
}

/**
 * Bit values — MUST match the WarningFlag enum in the firmware's
 * include/VehicleEnums.h exactly. Kept as Long (not an enum/sealed class)
 * because the wire format is a raw bitmask and multiple flags are commonly
 * set simultaneously.
 */
object WarningFlags {
    const val CHECK_ENGINE: Long = 1L shl 0
    const val OIL_PRESSURE: Long = 1L shl 1
    const val ENGINE_OVERTEMP: Long = 1L shl 2
    const val BATTERY_LOW: Long = 1L shl 3
    const val CHARGING_FAULT: Long = 1L shl 4
    const val FUEL_LOW: Long = 1L shl 5
    const val ABS_FAULT: Long = 1L shl 6
    const val SERVICE_DUE: Long = 1L shl 7
    const val TYRE_DUE: Long = 1L shl 8
    const val CHAIN_LUBE_DUE: Long = 1L shl 9
    const val INSURANCE_EXPIRING: Long = 1L shl 10
    const val PUC_EXPIRING: Long = 1L shl 11
    const val CRASH_DETECTED: Long = 1L shl 12
    const val UNAUTHORIZED_MOVE: Long = 1L shl 13
    const val GPS_LOST: Long = 1L shl 14
    const val SD_CARD_FAULT: Long = 1L shl 15

    /** Human-readable labels for whichever flags are set — used by the Dashboard's warning list. */
    fun activeLabels(warn: Long): List<String> = buildList {
        if (warn and CHECK_ENGINE != 0L) add("Check Engine")
        if (warn and OIL_PRESSURE != 0L) add("Oil Pressure Low")
        if (warn and ENGINE_OVERTEMP != 0L) add("Engine Overheating")
        if (warn and BATTERY_LOW != 0L) add("Battery Low")
        if (warn and CHARGING_FAULT != 0L) add("Charging Fault")
        if (warn and FUEL_LOW != 0L) add("Fuel Low")
        if (warn and ABS_FAULT != 0L) add("ABS Fault")
        if (warn and SERVICE_DUE != 0L) add("Service Due")
        if (warn and TYRE_DUE != 0L) add("Tyre Change Due")
        if (warn and CHAIN_LUBE_DUE != 0L) add("Chain Lube Due")
        if (warn and INSURANCE_EXPIRING != 0L) add("Insurance Expiring")
        if (warn and PUC_EXPIRING != 0L) add("PUC Expiring")
        if (warn and CRASH_DETECTED != 0L) add("Crash Detected")
        if (warn and UNAUTHORIZED_MOVE != 0L) add("Unauthorized Movement")
        if (warn and GPS_LOST != 0L) add("GPS Signal Lost")
        if (warn and SD_CARD_FAULT != 0L) add("SD Card Error")
    }
}
