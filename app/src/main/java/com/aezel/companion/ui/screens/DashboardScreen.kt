package com.aezel.companion.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.aezel.companion.ble.BleConnectionState
import com.aezel.companion.data.VehicleState
import com.aezel.companion.data.WarningFlags
import com.aezel.companion.ui.theme.AezelSuccess
import com.aezel.companion.ui.theme.AezelTextSecondary
import com.aezel.companion.ui.theme.AezelWarning

@Composable
fun DashboardScreen(vehicleState: VehicleState, connectionState: BleConnectionState) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
    ) {
        ConnectionBadge(connectionState)
        Spacer(Modifier.height(16.dp))

        if (connectionState !is BleConnectionState.Connected) {
            EmptyState()
            return@Column
        }

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceEvenly,
        ) {
            BigStat(label = "km/h", value = vehicleState.speedKmh.toString())
            BigStat(label = "RPM", value = vehicleState.rpm.toString())
        }

        Spacer(Modifier.height(24.dp))

        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            SmallStat("Fuel", "${vehicleState.fuelPct}%")
            SmallStat("Battery", "%.1fV".format(vehicleState.batteryVoltage))
            SmallStat("Engine", "%.0f\u00b0C".format(vehicleState.engineTempC))
            SmallStat("Odometer", "%.0f km".format(vehicleState.odometerKm))
        }

        Spacer(Modifier.height(24.dp))

        val activeWarnings = WarningFlags.activeLabels(vehicleState.warningFlags)
        if (activeWarnings.isNotEmpty()) {
            Text("Active Warnings", style = MaterialTheme.typography.titleLarge, color = AezelWarning)
            Spacer(Modifier.height(8.dp))
            LazyColumn {
                items(activeWarnings) { label ->
                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 4.dp),
                        colors = CardDefaults.cardColors(containerColor = AezelWarning.copy(alpha = 0.15f)),
                    ) {
                        Text(label, modifier = Modifier.padding(12.dp), color = AezelWarning)
                    }
                }
            }
        } else {
            Text("No active warnings", color = AezelSuccess)
        }
    }
}

@Composable
private fun ConnectionBadge(state: BleConnectionState) {
    val (label, color) = when (state) {
        is BleConnectionState.Connected -> "Connected to ${state.deviceName ?: "AEZEL"}" to AezelSuccess
        is BleConnectionState.Connecting -> "Connecting\u2026" to AezelTextSecondary
        is BleConnectionState.Scanning -> "Scanning\u2026" to AezelTextSecondary
        is BleConnectionState.Failed -> state.reason to AezelWarning
        BleConnectionState.Disconnected -> "Not connected" to AezelTextSecondary
    }
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(label, color = color, style = MaterialTheme.typography.bodyMedium)
    }
}

@Composable
private fun EmptyState() {
    Column(
        modifier = Modifier.fillMaxSize(),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Text("Not connected to your bike", style = MaterialTheme.typography.titleLarge, color = AezelTextSecondary)
        Spacer(Modifier.height(8.dp))
        Text(
            "Go to the Device tab to scan and connect",
            style = MaterialTheme.typography.bodyMedium,
            color = AezelTextSecondary,
        )
    }
}

@Composable
private fun BigStat(label: String, value: String) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Text(value, fontSize = 56.sp, fontWeight = FontWeight.Bold)
        Text(label, color = AezelTextSecondary, style = MaterialTheme.typography.bodyMedium)
    }
}

@Composable
private fun SmallStat(label: String, value: String) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Text(value, fontSize = 18.sp, fontWeight = FontWeight.SemiBold)
        Text(label, color = AezelTextSecondary, style = MaterialTheme.typography.labelSmall)
    }
}
