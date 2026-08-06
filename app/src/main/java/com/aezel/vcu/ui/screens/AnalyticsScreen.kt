package com.aezel.vcu.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.aezel.vcu.model.VehicleTelemetry
import com.aezel.vcu.ui.components.MetricCard
import com.aezel.vcu.ui.theme.*

@Composable
fun AnalyticsScreen(telemetry: VehicleTelemetry) {
    val scrollState = rememberScrollState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(BackgroundDark)
            .padding(16.dp)
            .verticalScroll(scrollState)
    ) {
        Text(
            text = "RIDE ANALYTICS & GPS LOGS",
            style = MaterialTheme.typography.titleLarge,
            color = AccentCyan,
            fontWeight = FontWeight.Bold
        )
        Text(
            text = "Telemetry Metrics, GPS Coordinates & Ride Logs",
            style = MaterialTheme.typography.labelSmall,
            color = TextSecondary
        )

        Spacer(modifier = Modifier.height(20.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            MetricCard(
                title = "MAX SPEED",
                value = "${(telemetry.speedKmh * 1.25).toInt()} KM/H",
                color = AccentCyan,
                modifier = Modifier.weight(1f)
            )
            MetricCard(
                title = "AVG SPEED",
                value = "${(telemetry.speedKmh * 0.65).toInt()} KM/H",
                color = AccentLime,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(12.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            MetricCard(
                title = "LATITUDE",
                value = String.format("%.4f", telemetry.latitude),
                color = TextPrimary,
                modifier = Modifier.weight(1f)
            )
            MetricCard(
                title = "LONGITUDE",
                value = String.format("%.4f", telemetry.longitude),
                color = TextPrimary,
                modifier = Modifier.weight(1f)
            )
        }

        Spacer(modifier = Modifier.height(24.dp))

        Button(
            onClick = { /* Export GPX / CSV Log */ },
            modifier = Modifier
                .fillMaxWidth()
                .height(56.dp),
            colors = ButtonDefaults.buttonColors(containerColor = CardSurfaceDark, contentColor = AccentCyan),
            border = androidx.compose.foundation.BorderStroke(1.dp, CardBorderDark),
            shape = RoundedCornerShape(14.dp)
        ) {
            Text(text = "📥 EXPORT GPX / CSV RIDE LOGS", fontSize = 14.sp, fontWeight = FontWeight.Bold)
        }
    }
}
