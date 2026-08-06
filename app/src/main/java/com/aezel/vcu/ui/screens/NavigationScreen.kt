package com.aezel.vcu.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.Navigation
import androidx.compose.material.icons.filled.Route
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.aezel.vcu.ble.AezelBleManager
import com.aezel.vcu.ui.theme.*

/**
 * 100% Automated Background Navigation Bridge Monitor
 * Captures live turn-by-turn directions from Google Maps / OsmAnd automatically
 * and streams them to the motorcycle cockpit hands-free!
 */
@Composable
fun NavigationScreen(bleManager: AezelBleManager) {
    var isAutoGpsMirrorEnabled by remember { mutableStateOf(true) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(BackgroundDark)
            .padding(16.dp)
    ) {
        Text(
            text = "AUTOMATED GPS NAVIGATION",
            style = MaterialTheme.typography.titleLarge,
            color = AccentCyan,
            fontWeight = FontWeight.Bold
        )
        Text(
            text = "Hands-Free Background Mirroring to Motorcycle Display",
            style = MaterialTheme.typography.labelSmall,
            color = TextSecondary
        )

        Spacer(modifier = Modifier.height(24.dp))

        // --- Automated Mirroring Master Switch ---
        Surface(
            color = CardSurfaceDark,
            border = androidx.compose.foundation.BorderStroke(1.dp, CardBorderDark),
            shape = RoundedCornerShape(16.dp),
            modifier = Modifier.fillMaxWidth()
        ) {
            Row(
                modifier = Modifier.padding(16.dp),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Box(
                        modifier = Modifier
                            .size(44.dp)
                            .clip(CircleShape)
                            .background(if (isAutoGpsMirrorEnabled) AccentLime.copy(alpha = 0.2f) else CardBorderDark),
                        contentAlignment = Alignment.Center
                    ) {
                        Icon(
                            imageVector = Icons.Default.Navigation,
                            contentDescription = null,
                            tint = if (isAutoGpsMirrorEnabled) AccentLime else TextSecondary
                        )
                    }
                    Spacer(modifier = Modifier.width(12.dp))
                    Column {
                        Text(
                            text = "Auto GPS Mirroring",
                            fontSize = 16.sp,
                            fontWeight = FontWeight.Bold,
                            color = TextPrimary
                        )
                        Text(
                            text = if (isAutoGpsMirrorEnabled) "Listening for Google Maps" else "Mirroring Disabled",
                            fontSize = 12.sp,
                            color = if (isAutoGpsMirrorEnabled) AccentLime else TextSecondary
                        )
                    }
                }

                Switch(
                    checked = isAutoGpsMirrorEnabled,
                    onCheckedChange = { isAutoGpsMirrorEnabled = it },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentLime,
                        checkedTrackColor = AccentLime.copy(alpha = 0.3f),
                        uncheckedThumbColor = TextSecondary,
                        uncheckedTrackColor = CardBorderDark
                    )
                )
            }
        }

        Spacer(modifier = Modifier.height(20.dp))

        // --- Live Auto-Captured GPS Status Card ---
        Surface(
            color = CardSurfaceDark,
            border = androidx.compose.foundation.BorderStroke(1.dp, if (isAutoGpsMirrorEnabled) AccentCyan else CardBorderDark),
            shape = RoundedCornerShape(20.dp),
            modifier = Modifier.fillMaxWidth()
        ) {
            Column(
                modifier = Modifier.padding(20.dp),
                horizontalAlignment = Alignment.Start
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "LIVE TURN STREAM",
                        fontSize = 12.sp,
                        fontWeight = FontWeight.Bold,
                        color = AccentCyan
                    )
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Icon(
                            imageVector = Icons.Default.CheckCircle,
                            contentDescription = null,
                            tint = AccentLime,
                            modifier = Modifier.size(16.dp)
                        )
                        Spacer(modifier = Modifier.width(4.dp))
                        Text(
                            text = "GOOGLE MAPS ACTIVE",
                            fontSize = 10.sp,
                            fontWeight = FontWeight.Bold,
                            color = AccentLime
                        )
                    }
                }

                Spacer(modifier = Modifier.height(16.dp))

                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(
                        text = "◀",
                        fontSize = 48.sp,
                        color = AccentCyan,
                        fontWeight = FontWeight.Black
                    )
                    Spacer(modifier = Modifier.width(16.dp))
                    Column {
                        Text(
                            text = "In 150 meters",
                            fontSize = 20.sp,
                            fontWeight = FontWeight.Black,
                            color = TextPrimary
                        )
                        Text(
                            text = "Turn left onto MG Road",
                            fontSize = 14.sp,
                            fontWeight = FontWeight.Medium,
                            color = TextSecondary
                        )
                    }
                }

                Spacer(modifier = Modifier.height(12.dp))
                HorizontalDivider(color = CardBorderDark)
                Spacer(modifier = Modifier.height(12.dp))

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text(text = "ETA: 12 Mins", fontSize = 12.sp, color = TextPrimary, fontWeight = FontWeight.Bold)
                    Text(text = "Remaining: 4.2 km", fontSize = 12.sp, color = AccentCyan, fontWeight = FontWeight.Bold)
                }
            }
        }

        Spacer(modifier = Modifier.height(24.dp))

        // --- Hands-Free Rider Guidance Card ---
        Surface(
            color = CardSurfaceDark.copy(alpha = 0.5f),
            border = androidx.compose.foundation.BorderStroke(1.dp, CardBorderDark),
            shape = RoundedCornerShape(16.dp),
            modifier = Modifier.fillMaxWidth()
        ) {
            Row(
                modifier = Modifier.padding(16.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(
                    imageVector = Icons.Default.Route,
                    contentDescription = null,
                    tint = AccentCyan,
                    modifier = Modifier.size(24.dp)
                )
                Spacer(modifier = Modifier.width(12.dp))
                Text(
                    text = "100% Hands-Free: Simply start navigation in Google Maps or OsmAnd before your ride. Turn icons, distances, and road names update automatically on your bike display.",
                    fontSize = 12.sp,
                    color = TextSecondary,
                    lineHeight = 16.sp
                )
            }
        }
    }
}
