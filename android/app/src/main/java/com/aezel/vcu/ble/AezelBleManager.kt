package com.aezel.vcu.ble

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.util.Log
import com.aezel.vcu.model.VehicleTelemetry
import com.google.gson.JsonObject
import com.google.gson.JsonParser
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import java.util.*

@SuppressLint("MissingPermission")
class AezelBleManager private constructor(private val context: Context) {

    companion object {
        const val TAG = "AezelBleManager"
        val SERVICE_UUID: UUID = UUID.fromString("6e400001-b5a3-f393-e0a9-e50e24dcca9e")
        val CHAR_TELEMETRY_UUID: UUID = UUID.fromString("6e400002-b5a3-f393-e0a9-e50e24dcca9e")
        val CHAR_COMMAND_UUID: UUID = UUID.fromString("6e400003-b5a3-f393-e0a9-e50e24dcca9e")

        @Volatile
        private var INSTANCE: AezelBleManager? = null

        fun getInstance(context: Context): AezelBleManager {
            return INSTANCE ?: synchronized(this) {
                INSTANCE ?: AezelBleManager(context.applicationContext).also { INSTANCE = it }
            }
        }
    }

    private val bluetoothAdapter: BluetoothAdapter? by lazy {
        val manager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        manager.adapter
    }

    private var bluetoothGatt: BluetoothGatt? = null
    private var commandChar: BluetoothGattCharacteristic? = null

    private val _telemetry = MutableStateFlow(VehicleTelemetry())
    val telemetry: StateFlow<VehicleTelemetry> = _telemetry

    private val _connectionState = MutableStateFlow("Disconnected")
    val connectionState: StateFlow<String> = _connectionState

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            result?.device?.let { device ->
                if (device.name == "Phoenix Cockpit" || device.name == "AEZEL VCU") {
                    Log.d(TAG, "Found AEZEL VCU: ${device.address}. Connecting...")
                    stopScan()
                    connectToDevice(device)
                }
            }
        }
    }

    fun startScan() {
        if (bluetoothAdapter?.isEnabled != true) {
            _connectionState.value = "Bluetooth Disabled"
            return
        }
        _connectionState.value = "Scanning..."
        bluetoothAdapter?.bluetoothLeScanner?.startScan(scanCallback)
    }

    fun stopScan() {
        bluetoothAdapter?.bluetoothLeScanner?.stopScan(scanCallback)
    }

    private fun connectToDevice(device: BluetoothDevice) {
        _connectionState.value = "Connecting..."
        bluetoothGatt = device.connectGatt(context, false, gattCallback)
    }

    fun disconnect() {
        bluetoothGatt?.disconnect()
        bluetoothGatt?.close()
        bluetoothGatt = null
        _connectionState.value = "Disconnected"
        _telemetry.value = _telemetry.value.copy(bleConnected = false)
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                _connectionState.value = "Connected"
                _telemetry.value = _telemetry.value.copy(bleConnected = true)
                Log.d(TAG, "GATT Connected. Discovering Services...")
                gatt?.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                _connectionState.value = "Disconnected"
                _telemetry.value = _telemetry.value.copy(bleConnected = false)
                Log.d(TAG, "GATT Disconnected.")
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                val service = gatt?.getService(SERVICE_UUID)
                commandChar = service?.getCharacteristic(CHAR_COMMAND_UUID)
                val telemetryChar = service?.getCharacteristic(CHAR_TELEMETRY_UUID)

                telemetryChar?.let { char ->
                    gatt.setCharacteristicNotification(char, true)
                    val descriptor = char.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"))
                    descriptor?.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                    gatt.writeDescriptor(descriptor)
                }
            }
        }

        override fun onCharacteristicChanged(gatt: BluetoothGatt?, characteristic: BluetoothGattCharacteristic?) {
            if (characteristic?.uuid == CHAR_TELEMETRY_UUID) {
                val jsonStr = String(characteristic.value ?: byteArrayOf(), Charsets.UTF_8)
                parseTelemetryJson(jsonStr)
            }
        }
    }

    private fun parseTelemetryJson(json: String) {
        try {
            val obj: JsonObject = JsonParser.parseString(json).asJsonObject
            val spd = obj.get("spd")?.asInt ?: 0
            val rpm = obj.get("rpm")?.asInt ?: 0
            val fuel = obj.get("fuel")?.asInt ?: 100
            val batt = obj.get("batt")?.asFloat ?: 12.6f
            val engT = obj.get("eng_t")?.asInt ?: 45
            val odo = obj.get("odo")?.asFloat ?: 0.0f
            val warn = obj.get("warn")?.asInt ?: 0
            val lat = obj.get("lat")?.asDouble ?: 0.0
            val lon = obj.get("lon")?.asDouble ?: 0.0

            CoroutineScope(Dispatchers.Main).launch {
                _telemetry.value = _telemetry.value.copy(
                    speedKmh = spd,
                    rpm = rpm,
                    fuelPct = fuel,
                    batteryVolts = batt,
                    engineTemp = engT,
                    odometer = odo,
                    warningsMask = warn,
                    latitude = lat,
                    longitude = lon,
                    bleConnected = true
                )
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error parsing telemetry JSON: ${e.message}")
        }
    }

    fun sendCommand(commandJson: String) {
        commandChar?.let { char ->
            char.value = commandJson.toByteArray(Charsets.UTF_8)
            bluetoothGatt?.writeCharacteristic(char)
            Log.d(TAG, "Sent BLE GATT Command: $commandJson")
        }
    }

    // High-Level Vehicle Remote Commands
    fun remoteEngineStart() {
        sendCommand("{\"cmd\":\"remote_start_engine\"}")
    }

    fun toggleIgnition() {
        sendCommand("{\"cmd\":\"remote_ignition_toggle\"}")
    }

    fun pulseHorn() {
        sendCommand("{\"cmd\":\"remote_horn_beep\"}")
    }

    fun toggleHazard() {
        sendCommand("{\"cmd\":\"remote_hazard_toggle\"}")
    }

    fun triggerSeatRelease() {
        sendCommand("{\"cmd\":\"remote_seat_release\"}")
    }

    fun triggerFindMyBike() {
        sendCommand("{\"cmd\":\"find_bike\"}")
    }

    fun sendNavigationUpdate(distMeters: Int, turnIcon: Int, streetName: String, etaMins: Int) {
        val payload = "{\"cmd\":\"nav_update\",\"dist\":$distMeters,\"turn\":$turnIcon,\"street\":\"$streetName\",\"eta\":$etaMins}"
        sendCommand(payload)
    }
}
