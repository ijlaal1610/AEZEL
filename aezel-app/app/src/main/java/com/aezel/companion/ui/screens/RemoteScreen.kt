package com.aezel.companion.ui.screens

import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.PressInteraction
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.aezel.companion.data.AezelRepository
import com.aezel.companion.data.VehicleState
import com.aezel.companion.ui.theme.AezelTextSecondary
import com.aezel.companion.ui.theme.AezelWarning

@Composable
fun RemoteScreen(repository: AezelRepository, vehicleState: VehicleState) {
    var showRemoteStartConfirm by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        Text("Remote Control", style = MaterialTheme.typography.titleLarge)

        if (vehicleState.commandResult.isNotBlank() && vehicleState.commandResult != "ok") {
            Card(colors = CardDefaults.cardColors(containerColor = AezelWarning.copy(alpha = 0.15f))) {
                Text(
                    "Last command: ${vehicleState.commandResult.replace('_', ' ')}",
                    modifier = Modifier.padding(12.dp),
                    color = AezelWarning,
                )
            }
        }

        Text("Find my bike", style = MaterialTheme.typography.bodyMedium, color = AezelTextSecondary)
        Button(onClick = { repository.findBike() }, modifier = Modifier.fillMaxWidth()) {
            Text("Honk + Flash Hazards")
        }

        HorizontalDivider()

        Text("Horn & Lights", style = MaterialTheme.typography.bodyMedium, color = AezelTextSecondary)
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            HoldToActivateButton(
                label = "Horn",
                modifier = Modifier.weight(1f),
                onPress = { repository.hornOn() },
                onRelease = { repository.hornOff() },
            )
            ToggleButton(
                label = "Hazards",
                modifier = Modifier.weight(1f),
                onOn = { repository.hazardOn() },
                onOff = { repository.hazardOff() },
            )
        }
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            ToggleButton(
                label = "Left Indicator",
                modifier = Modifier.weight(1f),
                onOn = { repository.indicatorLeftOn() },
                onOff = { repository.indicatorLeftOff() },
            )
            ToggleButton(
                label = "Right Indicator",
                modifier = Modifier.weight(1f),
                onOn = { repository.indicatorRightOn() },
                onOff = { repository.indicatorRightOff() },
            )
        }
        Text(
            "Indicators/hazards are refused by the dashboard while the bike is moving \u2014 this is enforced on the bike, not just in this app.",
            style = MaterialTheme.typography.labelSmall,
            color = AezelTextSecondary,
        )

        HorizontalDivider()

        Text("Lock", style = MaterialTheme.typography.bodyMedium, color = AezelTextSecondary)
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(onClick = { repository.lock() }, modifier = Modifier.weight(1f)) { Text("Lock") }
            Button(onClick = { repository.unlock() }, modifier = Modifier.weight(1f)) { Text("Unlock") }
        }
        Text(
            "Locking only works when the bike is stationary with the engine off \u2014 refused otherwise, by design.",
            style = MaterialTheme.typography.labelSmall,
            color = AezelTextSecondary,
        )

        HorizontalDivider()

        Text("Remote Start", style = MaterialTheme.typography.bodyMedium, color = AezelTextSecondary)
        Text(
            "Only works if the bike is in neutral, side stand down, and kill switch off \u2014 checked on the bike itself. Auto-stops after 5 minutes if left unattended.",
            style = MaterialTheme.typography.labelSmall,
            color = AezelTextSecondary,
        )
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(
                onClick = { showRemoteStartConfirm = true },
                modifier = Modifier.weight(1f),
                colors = ButtonDefaults.buttonColors(containerColor = AezelWarning),
            ) { Text("Start Engine") }
            OutlinedButton(onClick = { repository.remoteStop() }, modifier = Modifier.weight(1f)) {
                Text("Stop Engine")
            }
        }
    }

    if (showRemoteStartConfirm) {
        AlertDialog(
            onDismissRequest = { showRemoteStartConfirm = false },
            title = { Text("Start engine remotely?") },
            text = {
                Text(
                    "The bike will only actually start if it's in neutral, the side stand is down, " +
                        "and the kill switch is off. Make sure nobody is near the bike before continuing.",
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    repository.remoteStart()
                    showRemoteStartConfirm = false
                }) { Text("Start") }
            },
            dismissButton = {
                TextButton(onClick = { showRemoteStartConfirm = false }) { Text("Cancel") }
            },
        )
    }
}

/**
 * Sends on-press / off-release rather than a single onClick — matches how
 * a real horn button behaves (sounds only while physically held). Uses
 * Button's interactionSource to observe press/release rather than a raw
 * pointerInput gesture detector, so it stays a real Material3 Button
 * (ripple, disabled-state handling, accessibility) rather than a
 * hand-rolled touch target.
 */
@Composable
private fun HoldToActivateButton(label: String, modifier: Modifier = Modifier, onPress: () -> Unit, onRelease: () -> Unit) {
    val interactionSource = remember { MutableInteractionSource() }
    LaunchedEffect(interactionSource) {
        interactionSource.interactions.collect { interaction ->
            when (interaction) {
                is PressInteraction.Press -> onPress()
                is PressInteraction.Release, is PressInteraction.Cancel -> onRelease()
            }
        }
    }
    Button(onClick = { /* real action happens via press/release above */ }, interactionSource = interactionSource, modifier = modifier) {
        Text(label)
    }
}

@Composable
private fun ToggleButton(label: String, modifier: Modifier = Modifier, onOn: () -> Unit, onOff: () -> Unit) {
    var isOn by remember { mutableStateOf(false) }
    OutlinedButton(
        onClick = {
            isOn = !isOn
            if (isOn) onOn() else onOff()
        },
        modifier = modifier,
        colors = if (isOn) {
            ButtonDefaults.outlinedButtonColors(containerColor = MaterialTheme.colorScheme.primary.copy(alpha = 0.2f))
        } else {
            ButtonDefaults.outlinedButtonColors()
        },
    ) { Text(label) }
}
