package com.aezel.companion.data

import android.content.Context
import com.aezel.companion.ble.BleConnectionManager
import com.aezel.companion.ble.BleConnectionState
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import org.json.JSONObject

/**
 * Single point of contact between the UI and the bike. Every command the
 * firmware understands (see docs/remote_control.md, docs/maintenance.md,
 * docs/security.md, docs/phone_link.md on the firmware side) has one
 * function here — the UI never builds a raw command JSON string itself.
 *
 * This is a plain singleton (not Hilt/Dagger) deliberately — the
 * dependency graph here is small (one BLE manager, no swappable
 * implementations needed), and pulling in a DI framework for an app this
 * size would be more ceremony than value. See docs/build.md if that
 * tradeoff should be revisited as the app grows.
 */
class AezelRepository private constructor(context: Context) {

    private val ble = BleConnectionManager(context.applicationContext)

    val connectionState: StateFlow<BleConnectionState> = ble.connectionState
    val telemetry: SharedFlow<VehicleState> = ble.telemetry

    private val _lastKnownState = MutableStateFlow(VehicleState())
    val lastKnownState: StateFlow<VehicleState> = _lastKnownState.asStateFlow()

    // Lives as long as the process (this is a process-wide singleton, see
    // getInstance() below) — deliberately not tied to any single screen's
    // lifecycle, since telemetry needs to keep updating lastKnownState
    // even while, e.g., the Settings screen (not Dashboard) is on top.
    private val repositoryScope = CoroutineScope(SupervisorJob() + Dispatchers.Default)

    init {
        repositoryScope.launch {
            ble.telemetry.collect { state -> _lastKnownState.value = state }
        }
    }

    val bleManager: BleConnectionManager get() = ble   // exposed for the scan/connect screen only

    // ---------------------------------------------------------- Trip -----
    fun resetTripA() = send(cmd("reset_trip_a"))
    fun resetTripB() = send(cmd("reset_trip_b"))

    // -------------------------------------------------- Remote control ---
    fun findBike() = send(cmd("find_bike"))
    fun hornOn() = send(cmd("horn_on"))
    fun hornOff() = send(cmd("horn_off"))
    fun hazardOn() = send(cmd("hazard_on"))
    fun hazardOff() = send(cmd("hazard_off"))
    fun indicatorLeftOn() = send(cmd("indicator_left_on"))
    fun indicatorLeftOff() = send(cmd("indicator_left_off"))
    fun indicatorRightOn() = send(cmd("indicator_right_on"))
    fun indicatorRightOff() = send(cmd("indicator_right_off"))
    fun lock() = send(cmd("lock"))
    fun unlock() = send(cmd("unlock"))
    /**
     * Remote-start is the highest-consequence command this app can send —
     * see docs/remote_control.md's Tier C for the interlocks the FIRMWARE
     * enforces (neutral, side stand down, kill switch off, not already
     * running). This function does not duplicate those checks — the
     * firmware is the source of truth and will refuse via `cmd_result` if
     * they're not met. The UI layer (RemoteScreen) is responsible for the
     * two-step confirmation gesture recommended in that doc; this function
     * intentionally has no confirmation of its own so that responsibility
     * stays visible at the call site rather than hidden in the repository.
     */
    fun remoteStart() = send(cmd("remote_start"))
    fun remoteStop() = send(cmd("remote_stop"))

    // ----------------------------------------------------- Maintenance ---
    fun markServiced() = send(cmd("mark_serviced"))
    fun markChainLubed() = send(cmd("mark_chain_lubed"))
    fun setTyreDueKm(km: Long) = send(cmd("set_tyre_due_km") { put("value", km) })
    fun setInsuranceDueEpochSeconds(epochSec: Long) = send(cmd("set_insurance_due_ts") { put("value", epochSec) })
    fun setPucDueEpochSeconds(epochSec: Long) = send(cmd("set_puc_due_ts") { put("value", epochSec) })

    // -------------------------------------------------------- Security ---
    fun armSecurity() = send(cmd("arm_security"))
    fun disarmSecurity() = send(cmd("disarm_security"))

    // ------------------------------------------------------- Phone link --
    // These are OUTBOUND from the app TO the dashboard — the reverse
    // direction (dashboard button presses coming back as "phone_action" in
    // telemetry) is handled by whoever observes `telemetry`/`lastKnownState`
    // (see MainActivity's phone-action dispatcher), not here.
    fun pushIncomingCall(callerName: String) = send(cmd("incoming_call") { put("caller", callerName) })
    fun pushCallEnded() = send(cmd("call_ended"))
    fun pushMessage(app: String, sender: String, preview: String) =
        send(cmd("push_message") { put("app", app); put("sender", sender); put("preview", preview) })
    fun pushMusicUpdate(title: String, artist: String, playing: Boolean) =
        send(cmd("music_update") { put("title", title); put("artist", artist); put("playing", playing) })
    fun pushNavUpdate(instruction: String, distanceM: Float) =
        send(cmd("nav_update") { put("instruction", instruction); put("distance_m", distanceM) })
    fun pushNavEnd() = send(cmd("nav_end"))

    private fun cmd(name: String, extra: (JSONObject.() -> Unit)? = null): String {
        val o = JSONObject()
        o.put("cmd", name)
        extra?.invoke(o)
        return o.toString()
    }

    private fun send(json: String): Boolean = ble.sendCommand(json)

    companion object {
        @Volatile private var instance: AezelRepository? = null

        fun getInstance(context: Context): AezelRepository =
            instance ?: synchronized(this) {
                instance ?: AezelRepository(context).also { instance = it }
            }
    }
}
