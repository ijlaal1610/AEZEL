package com.aezel.companion.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.aezel.companion.data.AezelRepository
import com.aezel.companion.data.VehicleState
import com.aezel.companion.ui.theme.AezelSuccess
import com.aezel.companion.ui.theme.AezelTextSecondary
import com.aezel.companion.ui.theme.AezelWarning

@Composable
fun SecurityScreen(repository: AezelRepository, vehicleState: VehicleState) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Text("Security", style = MaterialTheme.typography.titleLarge)

        val (label, color) = when (vehicleState.securityState) {
            "armed" -> "Armed" to AezelSuccess
            "alarm_active" -> "ALARM TRIGGERED" to AezelWarning
            "disarmed" -> "Disarmed" to AezelTextSecondary
            else -> "Unknown" to AezelTextSecondary
        }

        Card(colors = CardDefaults.cardColors(containerColor = color.copy(alpha = 0.15f))) {
            Column(modifier = Modifier.padding(16.dp), horizontalAlignment = Alignment.CenterHorizontally) {
                Text(label, style = MaterialTheme.typography.headlineLarge, color = color)
            }
        }

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(
                onClick = { repository.armSecurity() },
                modifier = Modifier.weight(1f),
                enabled = vehicleState.securityState != "armed" && vehicleState.securityState != "alarm_active",
            ) { Text("Arm") }
            OutlinedButton(
                onClick = { repository.disarmSecurity() },
                modifier = Modifier.weight(1f),
                enabled = vehicleState.securityState != "disarmed",
            ) { Text("Disarm") }
        }

        Text(
            "Arming only works when the bike is stationary with the engine off. " +
                "Once armed, wheel motion (and lean-angle or GPS drift if that hardware is installed) " +
                "triggers a horn + hazard alarm that auto-quiets after 30 seconds \u2014 " +
                "the warning itself stays until you disarm.",
            style = MaterialTheme.typography.bodyMedium,
            color = AezelTextSecondary,
        )

        Text(
            "This is a local alarm + notification, not a cellular tracker. If your phone isn't in " +
                "Bluetooth range when it triggers, you'll find out on the next reconnect, not instantly.",
            style = MaterialTheme.typography.labelSmall,
            color = AezelTextSecondary,
        )
    }
}
