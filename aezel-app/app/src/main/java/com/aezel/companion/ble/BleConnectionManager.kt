package com.aezel.companion.ble

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Build
import android.util.Log
import com.aezel.companion.data.VehicleState
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.flow.asStateFlow
import java.nio.charset.StandardCharsets

sealed class BleConnectionState {
    object Disconnected : BleConnectionState()
    object Scanning : BleConnectionState()
    data class Connecting(val deviceName: String?) : BleConnectionState()
    data class Connected(val deviceName: String?, val deviceAddress: String) : BleConnectionState()
    data class Failed(val reason: String) : BleConnectionState()
}

/**
 * Owns the entire BLE lifecycle: scan for the AEZEL service UUID, connect,
 * discover services, negotiate a larger MTU (the default 23-byte BLE MTU
 * is smaller than the firmware's telemetry JSON payload — without this
 * negotiation, notify packets arrive truncated), subscribe to telemetry
 * notifications, and expose a simple sendCommand() for writes.
 *
 * Deliberately uses Android's BluetoothGatt API directly rather than a
 * third-party BLE library — see app/build.gradle.kts for why: this app can
 * send lock/remote-start commands, and the reconnection/retry behavior of
 * a GATT client is not something to outsource to a dependency without
 * being able to fully audit it.
 *
 * All Bluetooth calls here assume BLUETOOTH_SCAN/BLUETOOTH_CONNECT (API 31+)
 * or the legacy BLUETOOTH/BLUETOOTH_ADMIN/ACCESS_FINE_LOCATION (API ≤30)
 * permissions have ALREADY been granted — callers (see
 * ui/screens/DeviceScanScreen.kt) are responsible for the runtime
 * permission request flow before invoking anything here. Calling these
 * without permission throws SecurityException on API 31+, which is
 * intentionally not caught here — a permission bug should be loud, not
 * silently swallowed.
 */
@SuppressLint("MissingPermission")   // permission responsibility is on the caller, see class doc above
class BleConnectionManager(private val context: Context) {

    companion object {
        private const val TAG = "AezelBle"
        private const val DESIRED_MTU = 247   // 244 usable bytes after ATT header — comfortably above the telemetry payload
        private const val SCAN_TIMEOUT_MS = 15_000L
    }

    private val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val adapter: BluetoothAdapter? get() = bluetoothManager.adapter

    private var gatt: BluetoothGatt? = null
    private var telemetryCharacteristic: BluetoothGattCharacteristic? = null
    private var commandCharacteristic: BluetoothGattCharacteristic? = null

    private val _connectionState = MutableStateFlow<BleConnectionState>(BleConnectionState.Disconnected)
    val connectionState: StateFlow<BleConnectionState> = _connectionState.asStateFlow()

    private val _telemetry = MutableSharedFlow<VehicleState>(replay = 1, extraBufferCapacity = 8)
    val telemetry: SharedFlow<VehicleState> = _telemetry.asSharedFlow()

    /** True once the telemetry characteristic's notify descriptor write has completed. */
    private var notificationsEnabled = false

    fun isBluetoothEnabled(): Boolean = adapter?.isEnabled == true

    /**
     * Scans specifically for AEZEL's advertised service UUID (see
     * AezelBleUuids.SERVICE) rather than a broad scan — faster, and avoids
     * the app needing to show/filter a list of unrelated nearby BLE
     * devices for what should be a one-bike pairing flow.
     */
    fun startScan(onDeviceFound: (BluetoothDevice) -> Unit, onScanFailed: (String) -> Unit) {
        val scanner = adapter?.bluetoothLeScanner
        if (scanner == null) {
            onScanFailed("Bluetooth adapter unavailable or disabled")
            return
        }
        _connectionState.value = BleConnectionState.Scanning

        val filter = ScanFilter.Builder()
            .setServiceUuid(android.os.ParcelUuid(AezelBleUuids.SERVICE))
            .build()
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        val callback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                scanner.stopScan(this)
                onDeviceFound(result.device)
            }
            override fun onScanFailed(errorCode: Int) {
                _connectionState.value = BleConnectionState.Failed("Scan failed (code $errorCode)")
                onScanFailed("Scan failed (code $errorCode)")
            }
        }
        scanner.startScan(listOf(filter), settings, callback)

        // Stop scanning after a timeout rather than draining battery scanning
        // forever if the bike's dashboard isn't powered on / in range.
        android.os.Handler(android.os.Looper.getMainLooper()).postDelayed({
            if (_connectionState.value is BleConnectionState.Scanning) {
                scanner.stopScan(callback)
                _connectionState.value = BleConnectionState.Failed("No AEZEL dashboard found nearby")
                onScanFailed("Timed out — no AEZEL dashboard found nearby")
            }
        }, SCAN_TIMEOUT_MS)
    }

    fun connect(device: BluetoothDevice) {
        _connectionState.value = BleConnectionState.Connecting(device.name)
        gatt = device.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
    }

    fun disconnect() {
        gatt?.disconnect()
        gatt?.close()
        gatt = null
        notificationsEnabled = false
        _connectionState.value = BleConnectionState.Disconnected
    }

    /**
     * Sends one JSON command — see docs/remote_control.md, docs/maintenance.md,
     * docs/security.md, and docs/phone_link.md on the firmware side for the
     * full command vocabulary this can send. Returns false immediately if
     * not connected/ready rather than queuing (a stale queued command —
     * e.g. an old horn_on sent after reconnecting minutes later — is worse
     * than a dropped one; the caller should retry explicitly if it matters).
     */
    fun sendCommand(json: String): Boolean {
        val g = gatt ?: return false
        val characteristic = commandCharacteristic ?: return false
        val bytes = json.toByteArray(StandardCharsets.UTF_8)

        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            g.writeCharacteristic(characteristic, bytes, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) == BluetoothStatusCodes.SUCCESS
        } else {
            @Suppress("DEPRECATION")
            characteristic.value = bytes
            @Suppress("DEPRECATION")
            characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            @Suppress("DEPRECATION")
            g.writeCharacteristic(characteristic)
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    Log.i(TAG, "Connected, discovering services")
                    g.requestMtu(DESIRED_MTU)   // triggers onMtuChanged -> discoverServices, see below
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    Log.i(TAG, "Disconnected (status=$status)")
                    notificationsEnabled = false
                    _connectionState.value = BleConnectionState.Disconnected
                    g.close()
                    gatt = null
                }
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            // Proceed to service discovery regardless of whether the MTU
            // request itself succeeded — a smaller MTU means telemetry
            // parsing may occasionally see truncated packets (handled
            // safely by VehicleState.parseOrNull returning null), not a
            // reason to abandon the connection entirely.
            Log.i(TAG, "MTU negotiated: $mtu (status=$status)")
            g.discoverServices()
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                _connectionState.value = BleConnectionState.Failed("Service discovery failed (status $status)")
                return
            }
            val service = g.getService(AezelBleUuids.SERVICE)
            if (service == null) {
                _connectionState.value = BleConnectionState.Failed("AEZEL service not found on this device")
                return
            }
            telemetryCharacteristic = service.getCharacteristic(AezelBleUuids.TELEMETRY_CHARACTERISTIC)
            commandCharacteristic = service.getCharacteristic(AezelBleUuids.COMMAND_CHARACTERISTIC)

            val telemetryChar = telemetryCharacteristic
            if (telemetryChar == null) {
                _connectionState.value = BleConnectionState.Failed("Telemetry characteristic missing")
                return
            }

            g.setCharacteristicNotification(telemetryChar, true)
            val cccd = telemetryChar.getDescriptor(AezelBleUuids.CLIENT_CHARACTERISTIC_CONFIG)
            if (cccd != null) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    g.writeDescriptor(cccd, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                } else {
                    @Suppress("DEPRECATION")
                    cccd.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                    @Suppress("DEPRECATION")
                    g.writeDescriptor(cccd)
                }
            }
        }

        override fun onDescriptorWrite(g: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            if (descriptor.uuid == AezelBleUuids.CLIENT_CHARACTERISTIC_CONFIG && status == BluetoothGatt.GATT_SUCCESS) {
                notificationsEnabled = true
                _connectionState.value = BleConnectionState.Connected(g.device.name, g.device.address)
                Log.i(TAG, "Notifications enabled, connection ready")
            }
        }

        // Pre-API-33 callback — value read from the characteristic itself.
        @Deprecated("Deprecated in Android API 33+, kept for minSdk 26 compatibility")
        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            if (characteristic.uuid != AezelBleUuids.TELEMETRY_CHARACTERISTIC) return
            @Suppress("DEPRECATION")
            handleTelemetryPayload(characteristic.value)
        }

        // API-33+ callback — value passed directly, preferred when available.
        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            if (characteristic.uuid != AezelBleUuids.TELEMETRY_CHARACTERISTIC) return
            handleTelemetryPayload(value)
        }

        private fun handleTelemetryPayload(bytes: ByteArray?) {
            if (bytes == null) return
            val json = String(bytes, StandardCharsets.UTF_8)
            val state = VehicleState.parseOrNull(json) ?: return   // malformed/truncated packet — drop silently, next tick (≤500ms later) will likely be fine
            _telemetry.tryEmit(state)
        }
    }
}
