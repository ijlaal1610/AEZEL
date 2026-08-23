package com.aezel.companion.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable

// Deliberately always dark, regardless of system theme — this app pairs
// with a dashboard that's read at a glance, often outdoors or at night;
// consistency with AEZEL's own always-legible dark theme matters more
// here than respecting the phone's light/dark system setting.
private val AezelColorScheme = darkColorScheme(
    primary = AezelAccent,
    secondary = AezelCaution,
    background = AezelBackground,
    surface = AezelSurface,
    error = AezelWarning,
    onPrimary = AezelBackground,
    onBackground = AezelTextPrimary,
    onSurface = AezelTextPrimary,
)

@Composable
fun AezelTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = AezelColorScheme,
        typography = AezelTypography,
        content = content,
    )
}
