package com.aezel.companion.data

import org.junit.Assert.*
import org.junit.Test

/**
 * Tests against the exact JSON shape BleManager::publishTelemetry() sends
 * on the firmware side (src/managers/BleManager.cpp) — kept as close to
 * real wire payloads as practical so a field-name drift between firmware
 * and app gets caught here rather than at 2am on a highway.
 */
class VehicleStateTest {

    @Test
    fun `parses a full realistic telemetry packet`() {
        val json = """
            {"spd":42,"rpm":3200,"fuel":58,"batt":12.6,"eng_t":78.5,"odo":4521.3,
             "warn":40,"lat":28.6139,"lon":77.2090,"cmd_result":"ok",
             "svc_due_km":7521,"chain_due_km":5021,"security_state":"armed"}
        """.trimIndent()

        val state = VehicleState.parseOrNull(json)

        assertNotNull(state)
        assertEquals(42, state!!.speedKmh)
        assertEquals(3200, state.rpm)
        assertEquals(58, state.fuelPct)
        assertEquals(12.6f, state.batteryVoltage, 0.01f)
        assertEquals(78.5f, state.engineTempC, 0.01f)
        assertEquals(4521.3f, state.odometerKm, 0.01f)
        assertEquals("armed", state.securityState)
        assertEquals(7521L, state.serviceDueKm)
        // warn=40 = 32 (FUEL_LOW, bit5) + 8 (BATTERY_LOW, bit3)
        assertTrue(state.hasWarning(WarningFlags.FUEL_LOW))
        assertTrue(state.hasWarning(WarningFlags.BATTERY_LOW))
        assertFalse(state.hasWarning(WarningFlags.ENGINE_OVERTEMP))
    }

    @Test
    fun `missing fields fall back to safe defaults rather than throwing`() {
        val state = VehicleState.parseOrNull("{}")
        assertNotNull(state)
        assertEquals(0, state!!.speedKmh)
        assertEquals(0L, state.warningFlags)
        assertFalse(state.callActive)
        assertNull(state.phoneAction)
    }

    @Test
    fun `malformed json returns null instead of throwing`() {
        assertNull(VehicleState.parseOrNull("not json at all"))
        assertNull(VehicleState.parseOrNull("{\"spd\":"))   // truncated, as a torn BLE packet might arrive
        assertNull(VehicleState.parseOrNull(""))
    }

    @Test
    fun `phone_action is null when absent, populated when present`() {
        val noAction = VehicleState.parseOrNull("""{"spd":0}""")
        assertNull(noAction!!.phoneAction)

        val withAction = VehicleState.parseOrNull("""{"spd":0,"phone_action":"call_accept"}""")
        assertEquals("call_accept", withAction!!.phoneAction)
    }

    @Test
    fun `warning flag bits match firmware VehicleEnums h exactly`() {
        // Spot-check a few — if these ever drift from include/VehicleEnums.h
        // on the firmware side, every warning label on the Dashboard would
        // silently be wrong, which is exactly what this guards against.
        assertEquals(1L, WarningFlags.CHECK_ENGINE)
        assertEquals(1L shl 5, WarningFlags.FUEL_LOW)
        assertEquals(1L shl 12, WarningFlags.CRASH_DETECTED)
        assertEquals(1L shl 15, WarningFlags.SD_CARD_FAULT)
    }

    @Test
    fun `activeLabels decodes multiple simultaneous flags`() {
        val combined = WarningFlags.FUEL_LOW or WarningFlags.BATTERY_LOW
        val labels = WarningFlags.activeLabels(combined)
        assertEquals(2, labels.size)
        assertTrue(labels.contains("Fuel Low"))
        assertTrue(labels.contains("Battery Low"))
    }

    @Test
    fun `activeLabels returns empty list when no warnings set`() {
        assertTrue(WarningFlags.activeLabels(0L).isEmpty())
    }
}
