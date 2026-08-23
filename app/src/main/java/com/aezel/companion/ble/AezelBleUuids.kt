package com.aezel.companion.ble

import java.util.UUID

/**
 * These MUST match src/managers/BleManager.cpp on the firmware side exactly
 * — SVC_UUID / CHAR_TELEMETRY / CHAR_COMMAND. If you regenerate the
 * firmware's UUIDs (recommended per that file's own comment, to avoid
 * clashing with anyone else's build on the same channel during
 * development), update these three to match.
 */
object AezelBleUuids {
    val SERVICE: UUID = UUID.fromString("6e400001-b5a3-f393-e0a9-e50e24dcca9e")
    val TELEMETRY_CHARACTERISTIC: UUID = UUID.fromString("6e400002-b5a3-f393-e0a9-e50e24dcca9e")   // notify, ESP32 -> phone
    val COMMAND_CHARACTERISTIC: UUID = UUID.fromString("6e400003-b5a3-f393-e0a9-e50e24dcca9e")     // write, phone -> ESP32

    // Standard BLE Client Characteristic Configuration Descriptor UUID —
    // writing to this is what actually enables notifications on the
    // telemetry characteristic. Not AEZEL-specific, this is the BLE spec's
    // fixed UUID for this purpose.
    val CLIENT_CHARACTERISTIC_CONFIG: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
}
