package com.aezel.companion.ui.screens

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.aezel.companion.ble.BleConnectionManager
import com.aezel.companion.ble.BleConnectionState
import com.aezel.companion.data.VehicleState
import com.aezel.companion.data.WarningFlags
import com.aezel.companion.ui.theme.*

/**
 * Gemini VCU Handlebar Cockpit Dashboard Screen
 * Renders a 60FPS digital instrument cluster and remote control grid for Gemini VCU!
 */
@Composable
fun DashboardScreen(
    vehicleState: VehicleState,
    connectionState: BleConnectionState,
    bleManager: BleConnectionManager? = null,
    isCockpitMode: Boolean = false,
    onToggleCockpitMode: () -> Unit = {}
) {
    val scrollState = rememberScrollState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(AezelBackgroundDark)
            .padding(if (isCockpitMode) 12.dp else 16.dp)
            .verticalScroll(scrollState),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        // --- Header / Connection Bar ---
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column {
                Text(
                    text = if (isCockpitMode) "🏍️ GEMINI COCKPIT" else "AEZEL GEMINI VCU",
                    style = MaterialTheme.typography.titleLarge,
                    color = AezelCyan,
                    fontWeight = FontWeight.Bold
                )
                ConnectionBadge(connectionState)
            }

            Row(verticalAlignment = Alignment.CenterVertically) {
                // Fullscreen Kiosk Mode Toggle Button
                IconButton(
                    onClick = onToggleCockpitMode,
                    modifier = Modifier
                        .clip(CircleShape)
                        .background(if (isCockpitMode) AezelCyan.copy(alpha = 0.25f) else AezelCardSurface)
                        .border(1.5.dp, if (isCockpitMode) AezelCyan else AezelCardBorder, CircleShape)
                ) {
                    Icon(
                        imageVector = if (isCockpitMode) Icons.Default.CloseFullscreen else Icons.Default.OpenInFull,
                        contentDescription = "Toggle Cockpit Kiosk Mode",
                        tint = if (isCockpitMode) AezelCyan else AezelTextPrimary
                    )
                }

                Spacer(modifier = Modifier.width(8.dp))

                // Gear Position Badge
                Box(
                    modifier = Modifier
                        .size(48.dp)
                        .clip(CircleShape)
                        .background(if (vehicleState.gear == "N") AezelSuccess.copy(alpha = 0.2f) else AezelCardSurface)
                        .border(2.dp, if (vehicleState.gear == "N") AezelSuccess else AezelCardBorder, CircleShape),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = vehicleState.gear,
                        fontSize = 22.sp,
                        fontWeight = FontWeight.Black,
                        color = if (vehicleState.gear == "N") AezelSuccess else AezelTextPrimary
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        // --- Main Circular Speedometer & 270° Tachometer Arc ---
        Box(
            modifier = Modifier
                .size(if (isCockpitMode) 290.dp else 240.dp)
                .padding(4.dp),
            contentAlignment = Alignment.Center
        ) {
            val speedAnim by animateFloatAsState(
                targetValue = vehicleState.speedKmh.toFloat(),
                animationSpec = tween(300)
            )
            val rpmAnim by animateFloatAsState(
                targetValue = vehicleState.rpm.toFloat(),
                animationSpec = tween(300)
            )

            Canvas(modifier = Modifier.fillMaxSize()) {
                // Background Gauge Track
                drawArc(
                    color = AezelCardBorder,
                    startAngle = 135f,
                    sweepAngle = 270f,
                    useCenter = false,
                    style = Stroke(width = 20.dp.toPx(), cap = StrokeCap.Round)
                )
                // Live Tachometer Sweep Arc (Redline Shift Light > 9500 RPM)
                val isRedline = rpmAnim > 9500f
                drawArc(
                    brush = Brush.sweepGradient(
                        if (isRedline) listOf(AezelRed, AezelRed)
                        else listOf(AezelCyan, AezelSuccess)
                    ),
                    startAngle = 135f,
                    sweepAngle = (rpmAnim / 12000f) * 270f,
                    useCenter = false,
                    style = Stroke(width = 20.dp.toPx(), cap = StrokeCap.Round)
                )
            }

            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    text = "${vehicleState.speedKmh}",
                    fontSize = if (isCockpitMode) 68.sp else 54.sp,
                    fontWeight = FontWeight.Black,
                    color = AezelTextPrimary
                )
                Text(
                    text = "KM/H",
                    fontSize = 14.sp,
                    fontWeight = FontWeight.Bold,
                    color = AezelCyan
                )
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    text = "${vehicleState.rpm} RPM",
                    fontSize = 13.sp,
                    color = if (vehicleState.rpm > 9500) AezelRed else AezelSuccess,
                    fontWeight = FontWeight.Bold
                )
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        // --- Active Warnings List ---
        val activeWarnings = WarningFlags.activeLabels(vehicleState.warningFlags)
        if (activeWarnings.isNotEmpty()) {
            LazyRow(
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                modifier = Modifier.fillMaxWidth()
            ) {
                items(activeWarnings) { warnLabel ->
                    Surface(
                        color = AezelRed.copy(alpha = 0.2f),
                        border = androidx.compose.foundation.BorderStroke(1.dp, AezelRed),
                        shape = RoundedCornerShape(20.dp)
                    ) {
                        Row(
                            modifier = Modifier.padding(horizontal = 12.dp, vertical = 6.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Icon(
                                Icons.Default.Warning,
                                contentDescription = null,
                                tint = AezelRed,
                                modifier = Modifier.size(16.dp)
                            )
                            Spacer(modifier = Modifier.width(6.dp))
                            Text(
                                text = warnLabel,
                                color = AezelRed,
                                fontSize = 12.sp,
                                fontWeight = FontWeight.Bold
                            )
                        }
                    }
                }
            }
            Spacer(modifier = Modifier.height(16.dp))
        }

        // --- Telemetry Cards Grid ---
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            MetricCard(
                title = "BATTERY",
                value = "%.1fV".format(vehicleState.batteryVoltage),
                color = if (vehicleState.batteryVoltage < 11.8f) AezelRed else AezelSuccess,
                modifier = Modifier.weight(1f)
            )
            MetricCard(
                title = "FUEL LEVEL",
                value = "${vehicleState.fuelPct}% (${vehicleState.fuelRangeKm}km)",
                color = if (vehicleState.fuelPct < 20) AezelWarning else AezelCyan,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(10.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            MetricCard(
                title = "ENGINE TEMP",
                value = "%.0f°C".format(vehicleState.engineTempC),
                color = if (vehicleState.engineTempC > 95) AezelRed else AezelTextPrimary,
                modifier = Modifier.weight(1f)
            )
            MetricCard(
                title = "ODOMETER",
                value = "%.0f KM".format(vehicleState.odometerKm),
                color = AezelCyan,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(10.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            MetricCard(
                title = "TRIP A",
                value = "%.1f KM".format(vehicleState.tripAKm),
                color = AezelTextPrimary,
                modifier = Modifier.weight(1f)
            )
            MetricCard(
                title = "LEAN ANGLE",
                value = "%.1f°".format(vehicleState.leanAngleDeg),
                color = AezelCyan,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(20.dp))

        // --- Handlebar Remote Control Grid ---
        Text(
            text = "⚡ HANDLEBAR REMOTE CONTROLS",
            style = MaterialTheme.typography.titleMedium,
            color = AezelTextPrimary,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.align(Alignment.Start)
        )

        Spacer(modifier = Modifier.height(10.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            Button(
                onClick = { bleManager?.remoteEngineStart() },
                colors = ButtonDefaults.buttonColors(containerColor = AezelSuccess),
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(12.dp)
            ) {
                Icon(Icons.Default.PlayArrow, contentDescription = null, tint = Color.Black)
                Spacer(modifier = Modifier.width(4.dp))
                Text("START ENGINE", color = Color.Black, fontWeight = FontWeight.Black, fontSize = 12.sp)
            }

            Button(
                onClick = { bleManager?.toggleIgnition() },
                colors = ButtonDefaults.buttonColors(containerColor = AezelCyan),
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(12.dp)
            ) {
                Icon(Icons.Default.PowerSettingsNew, contentDescription = null, tint = Color.Black)
                Spacer(modifier = Modifier.width(4.dp))
                Text("IGNITION", color = Color.Black, fontWeight = FontWeight.Bold, fontSize = 12.sp)
            }
        }

        Spacer(modifier = Modifier.height(10.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            OutlinedButton(
                onClick = { bleManager?.pulseHorn() },
                border = androidx.compose.foundation.BorderStroke(1.dp, AezelRed),
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(12.dp)
            ) {
                Text("📣 HORN", color = AezelRed, fontWeight = FontWeight.Bold, fontSize = 12.sp)
            }

            OutlinedButton(
                onClick = { bleManager?.toggleHazard() },
                border = androidx.compose.foundation.BorderStroke(1.dp, AezelWarning),
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(12.dp)
            ) {
                Text("🚨 HAZARDS", color = AezelWarning, fontWeight = FontWeight.Bold, fontSize = 12.sp)
            }

            OutlinedButton(
                onClick = { bleManager?.triggerSeatRelease() },
                border = androidx.compose.foundation.BorderStroke(1.dp, AezelCyan),
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(12.dp)
            ) {
                Text("🔓 SEAT LOCK", color = AezelCyan, fontWeight = FontWeight.Bold, fontSize = 12.sp)
            }
        }
    }
}

@Composable
private fun MetricCard(
    title: String,
    value: String,
    color: Color,
    modifier: Modifier = Modifier
) {
    Surface(
        modifier = modifier,
        color = AezelCardSurface,
        border = androidx.compose.foundation.BorderStroke(1.dp, AezelCardBorder),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(
            modifier = Modifier.padding(14.dp),
            horizontalAlignment = Alignment.Start
        ) {
            Text(
                text = title,
                fontSize = 10.sp,
                color = AezelTextSecondary,
                fontWeight = FontWeight.Bold
            )
            Spacer(modifier = Modifier.height(4.dp))
            Text(
                text = value,
                fontSize = 18.sp,
                color = color,
                fontWeight = FontWeight.Black
            )
        }
    }
}

@Composable
private fun ConnectionBadge(state: BleConnectionState) {
    val (label, color) = when (state) {
        is BleConnectionState.Connected -> "Connected (${state.deviceName ?: "Gemini VCU"})" to AezelSuccess
        is BleConnectionState.Connecting -> "Connecting\u2026" to AezelTextSecondary
        is BleConnectionState.Scanning -> "Scanning\u2026" to AezelTextSecondary
        is BleConnectionState.Failed -> state.reason to AezelWarning
        BleConnectionState.Disconnected -> "Not connected" to AezelTextSecondary
    }
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(label, color = color, style = MaterialTheme.typography.bodyMedium)
    }
}
