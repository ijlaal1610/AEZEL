package com.aezel.vcu.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.aezel.vcu.ble.AezelBleManager
import com.aezel.vcu.ui.theme.*

@Composable
fun NavigationScreen(bleManager: AezelBleManager) {
    var streetName by remember { mutableStateOf("MG Road") }
    var distanceMeters by remember { mutableStateOf(150f) }
    var selectedTurnIcon by remember { mutableStateOf(1) } // 0: Straight, 1: Left, 2: Right, 3: U-Turn
    var etaMinutes by remember { mutableStateOf(12f) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(BackgroundDark)
            .padding(16.dp)
    ) {
        Text(
            text = "TURN-BY-TURN NAVIGATION ENGINE",
            style = MaterialTheme.typography.titleLarge,
            color = AccentCyan,
            fontWeight = FontWeight.Bold
        )
        Text(
            text = "Push GPS Navigation Directions to Motorcycle Dashboard",
            style = MaterialTheme.typography.labelSmall,
            color = TextSecondary
        )

        Spacer(modifier = Modifier.height(24.dp))

        // Target Street Name Input
        OutlinedTextField(
            value = streetName,
            onValueChange = { streetName = it },
            label = { Text("Target Street / Highway Name") },
            modifier = Modifier.fillMaxWidth(),
            colors = OutlinedTextFieldDefaults.colors(
                focusedBorderColor = AccentCyan,
                unfocusedBorderColor = CardBorderDark,
                focusedTextColor = TextPrimary,
                unfocusedTextColor = TextPrimary
            )
        )

        Spacer(modifier = Modifier.height(16.dp))

        // Turn Direction Selector
        Text(text = "Turn Direction Icon", fontSize = 14.sp, color = TextSecondary, fontWeight = FontWeight.Bold)
        Spacer(modifier = Modifier.height(8.dp))
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            val turns = listOf("⬆ Straight", "◀ Left", "▶ Right", "↩ U-Turn")
            turns.forEachIndexed { index, label ->
                FilterChip(
                    selected = selectedTurnIcon == index,
                    onClick = { selectedTurnIcon = index },
                    label = { Text(label, fontWeight = FontWeight.Bold) },
                    modifier = Modifier.weight(1f),
                    colors = FilterChipDefaults.filterChipColors(
                        selectedContainerColor = AccentCyan,
                        selectedLabelColor = BackgroundDark,
                        containerColor = CardSurfaceDark,
                        labelColor = TextPrimary
                    )
                )
            }
        }

        Spacer(modifier = Modifier.height(20.dp))

        // Distance Slider
        Text(text = "Distance to Turn: ${distanceMeters.toInt()} meters", fontSize = 14.sp, color = TextSecondary, fontWeight = FontWeight.Bold)
        Slider(
            value = distanceMeters,
            onValueChange = { distanceMeters = it },
            valueRange = 10f..2000f,
            colors = SliderDefaults.colors(thumbColor = AccentCyan, activeTrackColor = AccentCyan)
        )

        Spacer(modifier = Modifier.height(16.dp))

        // ETA Slider
        Text(text = "Estimated Time of Arrival: ${etaMinutes.toInt()} Mins", fontSize = 14.sp, color = TextSecondary, fontWeight = FontWeight.Bold)
        Slider(
            value = etaMinutes,
            onValueChange = { etaMinutes = it },
            valueRange = 1f..60f,
            colors = SliderDefaults.colors(thumbColor = AccentLime, activeTrackColor = AccentLime)
        )

        Spacer(modifier = Modifier.weight(1f))

        // Push to Dashboard Button
        Button(
            onClick = {
                bleManager.sendNavigationUpdate(
                    distMeters = distanceMeters.toInt(),
                    turnIcon = selectedTurnIcon,
                    streetName = streetName,
                    etaMins = etaMinutes.toInt()
                )
            },
            modifier = Modifier
                .fillMaxWidth()
                .height(56.dp),
            colors = ButtonDefaults.buttonColors(containerColor = AccentCyan, contentColor = BackgroundDark),
            shape = RoundedCornerShape(14.dp)
        ) {
            Text(text = "🗺️ PUSH TO MOTORCYCLE DASHBOARD", fontSize = 16.sp, fontWeight = FontWeight.Black)
        }
    }
}
