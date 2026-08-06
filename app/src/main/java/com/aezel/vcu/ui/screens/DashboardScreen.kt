package com.aezel.vcu.ui.screens

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.aezel.vcu.ble.AezelBleManager
import com.aezel.vcu.model.VehicleTelemetry
import com.aezel.vcu.model.decodeWarningFlags
import com.aezel.vcu.ui.theme.*

/**
 * AEZEL Detachable Cockpit Dashboard Screen
 * Allows using your smartphone as a full 60fps digital instrument cluster
 * when the physical screen is detached or not installed!
 */
@Composable
fun DashboardScreen(telemetry: VehicleTelemetry) {
    val scrollState = rememberScrollState()
    var isHandlebarCockpitMode by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(BackgroundDark)
            .padding(16.dp)
            .verticalScroll(scrollState),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        // --- Header Status Bar & Mode Switcher ---
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column {
                Text(
                    text = if (isHandlebarCockpitMode) "🏍️ HANDLEBAR COCKPIT" else "AEZEL DIGITAL VCU",
                    style = MaterialTheme.typography.titleLarge,
                    color = AccentCyan,
                    fontWeight = FontWeight.Bold
                )
                Text(
                    text = if (telemetry.bleConnected) "CONNECTED (BLE GATT)" else "SEARCHING FOR BIKE...",
                    style = MaterialTheme.typography.labelSmall,
                    color = if (telemetry.bleConnected) AccentLime else AccentAmber
                )
            }

            Row(verticalAlignment = Alignment.CenterVertically) {
                // Detachable Cockpit Mode Toggle Button
                IconButton(
                    onClick = { isHandlebarCockpitMode = !isHandlebarCockpitMode },
                    modifier = Modifier
                        .clip(CircleShape)
                        .background(if (isHandlebarCockpitMode) AccentCyan.copy(alpha = 0.2f) else CardSurfaceDark)
                        .border(1.dp, if (isHandlebarCockpitMode) AccentCyan else CardBorderDark, CircleShape)
                ) {
                    Icon(
                        imageVector = if (isHandlebarCockpitMode) Icons.Default.Smartphone else Icons.Default.DirectionsBike,
                        contentDescription = "Toggle Cockpit Mode",
                        tint = if (isHandlebarCockpitMode) AccentCyan else TextPrimary
                    )
                }

                Spacer(modifier = Modifier.width(8.dp))

                // Gear Position Badge
                Box(
                    modifier = Modifier
                        .size(48.dp)
                        .clip(CircleShape)
                        .background(if (telemetry.gear == "N") AccentLime.copy(alpha = 0.2f) else CardSurfaceDark)
                        .border(2.dp, if (telemetry.gear == "N") AccentLime else CardBorderDark, CircleShape),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = telemetry.gear,
                        fontSize = 22.sp,
                        fontWeight = FontWeight.Black,
                        color = if (telemetry.gear == "N") AccentLime else TextPrimary
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(20.dp))

        // --- Main Circular Speedometer & 270° Tachometer Gauge ---
        Box(
            modifier = Modifier
                .size(if (isHandlebarCockpitMode) 280.dp else 240.dp)
                .padding(8.dp),
            contentAlignment = Alignment.Center
        ) {
            val speedAnim by animateFloatAsState(
                targetValue = telemetry.speedKmh.toFloat(),
                animationSpec = tween(300)
            )
            val rpmAnim by animateFloatAsState(
                targetValue = telemetry.rpm.toFloat(),
                animationSpec = tween(300)
            )

            Canvas(modifier = Modifier.fillMaxSize()) {
                // Background Track
                drawArc(
                    color = CardBorderDark,
                    startAngle = 135f,
                    sweepAngle = 270f,
                    useCenter = false,
                    style = Stroke(width = 18.dp.toPx(), cap = StrokeCap.Round)
                )
                // Tachometer Sweep Arc (Redline shift light threshold > 9500 RPM)
                val isRedline = rpmAnim > 9500f
                drawArc(
                    brush = Brush.sweepGradient(
                        if (isRedline) listOf(AccentRed, AccentRed)
                        else listOf(AccentCyan, AccentLime)
                    ),
                    startAngle = 135f,
                    sweepAngle = (rpmAnim / 12000f) * 270f,
                    useCenter = false,
                    style = Stroke(width = 18.dp.toPx(), cap = StrokeCap.Round)
                )
            }

            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    text = "${telemetry.speedKmh}",
                    fontSize = if (isHandlebarCockpitMode) 64.sp else 54.sp,
                    fontWeight = FontWeight.Black,
                    color = TextPrimary
                )
                Text(
                    text = "KM/H",
                    fontSize = 14.sp,
                    fontWeight = FontWeight.Bold,
                    color = AccentCyan
                )
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    text = "${telemetry.rpm} RPM",
                    fontSize = 13.sp,
                    color = if (telemetry.rpm > 9500) AccentRed else AccentLime,
                    fontWeight = FontWeight.Bold
                )
            }
        }

        Spacer(modifier = Modifier.height(20.dp))

        // --- Active Warnings Carousel ---
        val warnings = decodeWarningFlags(telemetry.warningsMask)
        if (warnings.isNotEmpty()) {
            LazyRow(
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                modifier = Modifier.fillMaxWidth()
            ) {
                items(warnings) { warnLabel ->
                    Surface(
                        color = AccentRed.copy(alpha = 0.2f),
                        border = androidx.compose.foundation.BorderStroke(1.dp, AccentRed),
                        shape = RoundedCornerShape(20.dp)
                    ) {
                        Row(
                            modifier = Modifier.padding(horizontal = 12.dp, vertical = 6.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Icon(
                                Icons.Default.Warning,
                                contentDescription = null,
                                tint = AccentRed,
                                modifier = Modifier.size(16.dp)
                            )
                            Spacer(modifier = Modifier.width(6.dp))
                            Text(
                                text = warnLabel,
                                color = AccentRed,
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
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            MetricCard(
                title = "BATTERY",
                value = "${telemetry.batteryVolts}V",
                color = if (telemetry.batteryVolts < 11.8f) AccentRed else AccentLime,
                modifier = Modifier.weight(1f)
            )
            MetricCard(
                title = "FUEL LEVEL",
                value = "${telemetry.fuelPct}% (${telemetry.fuelRangeKm} km)",
                color = if (telemetry.fuelPct < 20) AccentAmber else AccentCyan,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(12.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            MetricCard(
                title = "ENGINE TEMP",
                value = "${telemetry.engineTemp}°C",
                color = if (telemetry.engineTemp > 95) AccentRed else TextPrimary,
                modifier = Modifier.weight(1f)
            )
            MetricCard(
                title = "ODOMETER",
                value = "${telemetry.odometer.toInt()} KM",
                color = AccentCyan,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(12.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            MetricCard(
                title = "TRIP A",
                value = "${telemetry.tripA} KM",
                color = TextPrimary,
                modifier = Modifier.weight(1f)
            )
            MetricCard(
                title = "MAX SPEED",
                value = "${telemetry.maxSpeedKmh} KM/H",
                color = AccentCyan,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(24.dp))

        // --- Full Handlebar Controls Grid ("Control Everything & More than Display") ---
        Text(
            text = "📱 HANDLEBAR REMOTE CONTROLS",
            style = MaterialTheme.typography.titleMedium,
            color = TextPrimary,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.align(Alignment.Start)
        )

        Spacer(modifier = Modifier.height(12.dp))

        val bleManager = AezelBleManager.getInstance(LocalContext.current)

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            // Remote Start Button
            Button(
                onClick = { bleManager.remoteEngineStart() },
                colors = ButtonDefaults.buttonColors(containerColor = AccentLime),
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(12.dp)
            ) {
                Icon(Icons.Default.PlayArrow, contentDescription = null, tint = Color.Black)
                Spacer(modifier = Modifier.width(4.dp))
                Text("START ENGINE", color = Color.Black, fontWeight = FontWeight.Black, fontSize = 12.sp)
            }

            // Keyless Ignition Toggle Button
            Button(
                onClick = { bleManager.toggleIgnition() },
                colors = ButtonDefaults.buttonColors(containerColor = AccentCyan),
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
            // Horn Beep
            OutlinedButton(
                onClick = { bleManager.pulseHorn() },
                border = androidx.compose.foundation.BorderStroke(1.dp, AccentRed),
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(12.dp)
            ) {
                Text("📣 HORN", color = AccentRed, fontWeight = FontWeight.Bold, fontSize = 12.sp)
            }

            // Hazards Toggle
            OutlinedButton(
                onClick = { bleManager.toggleHazard() },
                border = androidx.compose.foundation.BorderStroke(1.dp, AccentAmber),
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(12.dp)
            ) {
                Text("🚨 HAZARDS", color = AccentAmber, fontWeight = FontWeight.Bold, fontSize = 12.sp)
            }

            // Seat Lock Release
            OutlinedButton(
                onClick = { bleManager.triggerSeatRelease() },
                border = androidx.compose.foundation.BorderStroke(1.dp, AccentCyan),
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(12.dp)
            ) {
                Text("🔓 SEAT LOCK", color = AccentCyan, fontWeight = FontWeight.Bold, fontSize = 12.sp)
            }
        }
    }
}

@Composable
fun MetricCard(
    title: String,
    value: String,
    color: Color,
    modifier: Modifier = Modifier
) {
    Surface(
        modifier = modifier,
        color = CardSurfaceDark,
        border = androidx.compose.foundation.BorderStroke(1.dp, CardBorderDark),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(
            modifier = Modifier.padding(14.dp),
            horizontalAlignment = Alignment.Start
        ) {
            Text(
                text = title,
                fontSize = 10.sp,
                color = TextSecondary,
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
