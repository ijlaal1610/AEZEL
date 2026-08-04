package com.aezel.vcu.ui.screens

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
import androidx.compose.material.icons.filled.FlashOn
import androidx.compose.material.icons.filled.Speed
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
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
import com.aezel.vcu.model.VehicleTelemetry
import com.aezel.vcu.model.decodeWarningFlags
import com.aezel.vcu.ui.theme.*

@Composable
fun DashboardScreen(telemetry: VehicleTelemetry) {
    val scrollState = rememberScrollState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(BackgroundDark)
            .padding(16.dp)
            .verticalScroll(scrollState),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        // --- Header Status Bar ---
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column {
                Text(
                    text = "AEZEL COCKPIT",
                    style = MaterialTheme.typography.titleLarge,
                    color = AccentCyan,
                    fontWeight = FontWeight.Bold
                )
                Text(
                    text = if (telemetry.bleConnected) "CONNECTED (BLE GATT)" else "SEARCHING FOR VCU...",
                    style = MaterialTheme.typography.labelSmall,
                    color = if (telemetry.bleConnected) AccentLime else AccentAmber
                )
            }

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

        Spacer(modifier = Modifier.height(24.dp))

        // --- Main Circular Speed & Tachometer Gauge ---
        Box(
            modifier = Modifier
                .size(240.dp)
                .padding(8.dp),
            contentAlignment = Alignment.Center
        ) {
            val speedAnim by animateFloatAsState(
                targetValue = telemetry.speedKmh.toFloat(),
                animationSpec = tween(400)
            )
            val rpmAnim by animateFloatAsState(
                targetValue = telemetry.rpm.toFloat(),
                animationSpec = tween(400)
            )

            Canvas(modifier = Modifier.fillMaxSize()) {
                // Background Track
                drawArc(
                    color = CardBorderDark,
                    startAngle = 135f,
                    sweepAngle = 270f,
                    useCenter = false,
                    style = Stroke(width = 16.dp.toPx(), cap = StrokeCap.Round)
                )
                // Speed Gauge Arc
                drawArc(
                    brush = Brush.sweepGradient(listOf(AccentCyan, AccentLime)),
                    startAngle = 135f,
                    sweepAngle = (speedAnim / 180f) * 270f,
                    useCenter = false,
                    style = Stroke(width = 16.dp.toPx(), cap = StrokeCap.Round)
                )
            }

            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    text = "${telemetry.speedKmh}",
                    fontSize = 54.sp,
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
                    fontSize = 12.sp,
                    color = AccentLime,
                    fontWeight = FontWeight.Medium
                )
            }
        }

        Spacer(modifier = Modifier.height(24.dp))

        // --- Active Warning Badges ---
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

        // --- Metrics Grid ---
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            // Battery Voltage Card
            MetricCard(
                title = "BATTERY",
                value = "${telemetry.batteryVolts}V",
                color = if (telemetry.batteryVolts < 11.8f) AccentRed else AccentLime,
                modifier = Modifier.weight(1f)
            )

            // Fuel Level Card
            MetricCard(
                title = "FUEL LEVEL",
                value = "${telemetry.fuelPct}%",
                color = if (telemetry.fuelPct < 20) AccentAmber else AccentCyan,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(12.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            // Engine Temperature Card
            MetricCard(
                title = "ENGINE TEMP",
                value = "${telemetry.engineTemp}°C",
                color = if (telemetry.engineTemp > 95) AccentRed else TextPrimary,
                modifier = Modifier.weight(1f)
            )

            // Odometer Card
            MetricCard(
                title = "ODOMETER",
                value = "${telemetry.odometer.toInt()} KM",
                color = AccentCyan,
                modifier = Modifier.weight(1f)
            )
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
            modifier = Modifier.padding(16.dp),
            horizontalAlignment = Alignment.Start
        ) {
            Text(
                text = title,
                fontSize = 11.sp,
                color = TextSecondary,
                fontWeight = FontWeight.Bold
            )
            Spacer(modifier = Modifier.height(6.dp))
            Text(
                text = value,
                fontSize = 20.sp,
                color = color,
                fontWeight = FontWeight.Black
            )
        }
    }
}
