package com.aezel.vcu.model

data class VehicleTelemetry(
    val speedKmh: Int = 0,
    val rpm: Int = 0,
    val fuelPct: Int = 100,
    val batteryVolts: Float = 12.6f,
    val engineTemp: Int = 45,
    val odometer: Float = 0.0f,
    val warningsMask: Int = 0,
    val latitude: Double = 0.0,
    val longitude: Double = 0.0,
    val bleConnected: Boolean = false,
    val ignitionOn: Boolean = false,
    val gear: String = "N"
)

enum class WarningFlag(val mask: Int, val label: String) {
    LOW_FUEL(1 shl 0, "LOW FUEL"),
    OVERHEAT(1 shl 1, "ENGINE OVERHEAT"),
    LOW_BATTERY(1 shl 2, "LOW BATTERY"),
    SIDE_STAND(1 shl 3, "SIDE STAND DOWN"),
    CRASH_DETECTED(1 shl 4, "CRASH DETECTED"),
    UNAUTHORIZED_MOVE(1 shl 5, "THEFT ALARM"),
    SERVICE_DUE(1 shl 6, "SERVICE DUE")
}

fun decodeWarningFlags(mask: Int): List<String> {
    return WarningFlag.values()
        .filter { (mask and it.mask) != 0 }
        .map { it.label }
}
