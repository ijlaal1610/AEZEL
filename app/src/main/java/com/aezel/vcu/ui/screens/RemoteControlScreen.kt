package com.aezel.vcu.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.aezel.vcu.ble.AezelBleManager
import com.aezel.vcu.model.VehicleTelemetry
import com.aezel.vcu.ui.theme.*

@Composable
fun RemoteControlScreen(bleManager: AezelBleManager, telemetry: VehicleTelemetry) {
    val scrollState = rememberScrollState()
    var showStartConfirmDialog by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(BackgroundDark)
            .padding(16.dp)
            .verticalScroll(scrollState)
    ) {
        Text(
            text = "SMARTPHONE REMOTE CONTROL",
            style = MaterialTheme.typography.titleLarge,
            color = AccentCyan,
            fontWeight = FontWeight.Bold
        )
        Text(
            text = "Bluetooth GATT Actuator & Keyless Command Center",
            style = MaterialTheme.typography.labelSmall,
            color = TextSecondary
        )

        Spacer(modifier = Modifier.height(20.dp))

        // --- PROMINENT REMOTE ENGINE START BUTTON ---
        Button(
            onClick = {
                if (telemetry.gear != "N") {
                    showStartConfirmDialog = true
                } else {
                    bleManager.remoteEngineStart()
                }
            },
            modifier = Modifier
                .fillMaxWidth()
                .height(64.dp),
            colors = ButtonDefaults.buttonColors(containerColor = Color.Transparent),
            contentPadding = PaddingValues(),
            shape = RoundedCornerShape(16.dp)
        ) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(
                        Brush.horizontalGradient(listOf(AccentEmerald, AccentCyan)),
                        shape = RoundedCornerShape(16.dp)
                    ),
                contentAlignment = Alignment.Center
            ) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(
                        Icons.Default.PlayArrow,
                        contentDescription = null,
                        tint = Color.White,
                        modifier = Modifier.size(28.dp)
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Text(
                        text = "🚀 REMOTE START ENGINE",
                        fontSize = 18.sp,
                        fontWeight = FontWeight.Black,
                        color = Color.White
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(20.dp))

        // --- Grid of Actuator Buttons ---
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            RemoteTile(
                title = "KEYLESS IGNITION",
                icon = Icons.Default.VpnKey,
                active = telemetry.ignitionOn,
                activeColor = AccentCyan,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.toggleIgnition() }
            )

            RemoteTile(
                title = "HORN PULSE",
                icon = Icons.Default.VolumeUp,
                active = false,
                activeColor = AccentRed,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.pulseHorn() }
            )
        }

        Spacer(modifier = Modifier.height(12.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            RemoteTile(
                title = "HAZARD FLASHERS",
                icon = Icons.Default.Warning,
                active = false,
                activeColor = AccentAmber,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.toggleHazard() }
            )

            RemoteTile(
                title = "SEAT LOCK RELEASE",
                icon = Icons.Default.LockOpen,
                active = false,
                activeColor = AccentEmerald,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.triggerSeatRelease() }
            )
        }

        Spacer(modifier = Modifier.height(12.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            RemoteTile(
                title = "◀ LEFT SIGNAL",
                icon = Icons.Default.ArrowBack,
                active = false,
                activeColor = AccentLime,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.indicatorLeft() }
            )

            RemoteTile(
                title = "RIGHT SIGNAL ▶",
                icon = Icons.Default.ArrowForward,
                active = false,
                activeColor = AccentLime,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.indicatorRight() }
            )
        }

        Spacer(modifier = Modifier.height(12.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            RemoteTile(
                title = "FIND MY BIKE ALARM",
                icon = Icons.Default.LocationOn,
                active = false,
                activeColor = AccentPurple,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.triggerFindMyBike() }
            )

            RemoteTile(
                title = "WI-FI OTA MODE",
                icon = Icons.Default.Wifi,
                active = false,
                activeColor = AccentCyan,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.triggerOtaWifi() }
            )
        }

        Spacer(modifier = Modifier.height(12.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            RemoteTile(
                title = "RESET TRIP A",
                icon = Icons.Default.Refresh,
                active = false,
                activeColor = TextSecondary,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.resetTripA() }
            )

            RemoteTile(
                title = "RESET TRIP B",
                icon = Icons.Default.Refresh,
                active = false,
                activeColor = TextSecondary,
                modifier = Modifier.weight(1f),
                onClick = { bleManager.resetTripB() }
            )
        }
    }

    // Safety Warning Dialog if trying to remote start while not in Neutral
    if (showStartConfirmDialog) {
        AlertDialog(
            onDismissRequest = { showStartConfirmDialog = false },
            title = {
                Text(text = "⚠️ SAFETY INTERLOCK VIOLATION", color = AccentRed, fontWeight = FontWeight.Bold)
            },
            text = {
                Text(text = "Vehicle is not in Neutral (Current Gear: ${telemetry.gear}). Shift to Neutral (N) before remote starting the engine to prevent runaway crashes.")
            },
            confirmButton = {
                TextButton(onClick = { showStartConfirmDialog = false }) {
                    Text("OK", color = AccentCyan)
                }
            },
            containerColor = CardSurfaceDark
        )
    }
}

@Composable
fun RemoteTile(
    title: String,
    icon: ImageVector,
    active: Boolean,
    activeColor: Color,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    Surface(
        onClick = onClick,
        modifier = modifier.height(110.dp),
        color = if (active) activeColor.copy(alpha = 0.15f) else CardSurfaceDark,
        border = androidx.compose.foundation.BorderStroke(1.dp, if (active) activeColor else CardBorderDark),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.SpaceBetween,
            horizontalAlignment = Alignment.Start
        ) {
            Icon(
                icon,
                contentDescription = null,
                tint = if (active) activeColor else TextSecondary,
                modifier = Modifier.size(28.dp)
            )
            Text(
                text = title,
                fontSize = 12.sp,
                color = if (active) activeColor else TextPrimary,
                fontWeight = FontWeight.Bold
            )
        }
    }
}
