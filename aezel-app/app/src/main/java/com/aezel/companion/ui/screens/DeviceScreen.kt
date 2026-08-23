package com.aezel.companion.ui.screens

import android.bluetooth.BluetoothDevice
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.aezel.companion.ble.BleConnectionState
import com.aezel.companion.data.AezelRepository
import com.aezel.companion.ui.theme.AezelTextSecondary
import com.aezel.companion.ui.theme.AezelWarning

@Composable
fun DeviceScreen(
    repository: AezelRepository,
    connectionState: BleConnectionState,
    onOpenNotificationAccessSettings: () -> Unit,
) {
    var scanError by remember { mutableStateOf<String?>(null) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Text("Device", style = MaterialTheme.typography.titleLarge)

        Card {
            Column(modifier = Modifier.padding(16.dp)) {
                Text("Connection", style = MaterialTheme.typography.bodyLarge)
                Spacer(Modifier.height(4.dp))
                Text(connectionStatusText(connectionState), color = AezelTextSecondary)
            }
        }

        when (connectionState) {
            is BleConnectionState.Connected -> {
                OutlinedButton(onClick = { repository.bleManager.disconnect() }, modifier = Modifier.fillMaxWidth()) {
                    Text("Disconnect")
                }
            }
            is BleConnectionState.Scanning, is BleConnectionState.Connecting -> {
                LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
            }
            else -> {
                Button(
                    onClick = {
                        scanError = null
                        repository.bleManager.startScan(
                            onDeviceFound = { device: BluetoothDevice -> repository.bleManager.connect(device) },
                            onScanFailed = { reason -> scanError = reason },
                        )
                    },
                    modifier = Modifier.fillMaxWidth(),
                ) { Text("Scan & Connect") }
            }
        }

        scanError?.let {
            Text(it, color = AezelWarning, style = MaterialTheme.typography.bodyMedium)
        }

        HorizontalDivider()

        Text("Calls, Messages & Music", style = MaterialTheme.typography.bodyLarge)
        Text(
            "To mirror calls, WhatsApp/SMS previews, and now-playing music on your dashboard, " +
                "this app needs Notification Access \u2014 Android requires this to be granted manually " +
                "in Settings, it can't be requested as a normal permission popup.",
            style = MaterialTheme.typography.bodyMedium,
            color = AezelTextSecondary,
        )
        OutlinedButton(onClick = onOpenNotificationAccessSettings, modifier = Modifier.fillMaxWidth()) {
            Text("Open Notification Access Settings")
        }

        HorizontalDivider()

        Text(
            "AEZEL Companion \u00b7 speaks the BLE protocol documented in the firmware's " +
                "docs/remote_control.md, docs/maintenance.md, docs/security.md, and docs/phone_link.md.",
            style = MaterialTheme.typography.labelSmall,
            color = AezelTextSecondary,
        )
    }
}

private fun connectionStatusText(state: BleConnectionState): String = when (state) {
    is BleConnectionState.Connected -> "Connected to ${state.deviceName ?: state.deviceAddress}"
    is BleConnectionState.Connecting -> "Connecting to ${state.deviceName ?: "device"}\u2026"
    is BleConnectionState.Scanning -> "Scanning for your AEZEL dashboard\u2026"
    is BleConnectionState.Failed -> state.reason
    BleConnectionState.Disconnected -> "Not connected"
}
