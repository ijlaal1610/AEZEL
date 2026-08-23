package com.aezel.companion.ui.screens

import android.app.DatePickerDialog
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.aezel.companion.data.AezelRepository
import com.aezel.companion.data.VehicleState
import com.aezel.companion.ui.theme.AezelTextSecondary
import com.aezel.companion.ui.theme.AezelWarning
import java.util.Calendar

@Composable
fun MaintenanceScreen(repository: AezelRepository, vehicleState: VehicleState) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Text("Maintenance", style = MaterialTheme.typography.titleLarge)

        // --- Service (odometer-based, self-scheduling on the firmware side) ---
        MaintenanceRow(
            title = "Service",
            statusText = dueStatusText(vehicleState.serviceDueKm, vehicleState.odometerKm, "km"),
            actionLabel = "Mark Serviced",
            onAction = { repository.markServiced() },
        )

        // --- Chain lube (same pattern as service) ---
        MaintenanceRow(
            title = "Chain Lube",
            statusText = dueStatusText(vehicleState.chainDueKm, vehicleState.odometerKm, "km"),
            actionLabel = "Mark Chain Lubed",
            onAction = { repository.markChainLubed() },
        )

        HorizontalDivider()

        Text(
            "Tyres, insurance, and PUC can't be guessed by the bike \u2014 they stay silent " +
                "until you set them here.",
            style = MaterialTheme.typography.bodyMedium,
            color = AezelTextSecondary,
        )

        // --- Tyre (odometer target, entered as a plain number) ---
        var tyreKmText by remember { mutableStateOf("") }
        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            OutlinedTextField(
                value = tyreKmText,
                onValueChange = { tyreKmText = it.filter { c -> c.isDigit() } },
                label = { Text("Tyre change due at (km)") },
                modifier = Modifier.weight(1f),
                singleLine = true,
            )
            Button(onClick = {
                tyreKmText.toLongOrNull()?.let { repository.setTyreDueKm(it) }
            }) { Text("Set") }
        }

        // --- Insurance / PUC (date pickers, converted to epoch seconds) ---
        DueDateRow(
            label = "Insurance expiry",
            onDatePicked = { epochSeconds -> repository.setInsuranceDueEpochSeconds(epochSeconds) },
        )
        DueDateRow(
            label = "PUC expiry",
            onDatePicked = { epochSeconds -> repository.setPucDueEpochSeconds(epochSeconds) },
        )

        Text(
            "Insurance/PUC warnings need the bike's clock to be set (via GPS) before they'll evaluate \u2014 " +
                "see the firmware's docs/maintenance.md.",
            style = MaterialTheme.typography.labelSmall,
            color = AezelTextSecondary,
        )
    }
}

@Composable
private fun MaintenanceRow(title: String, statusText: String, actionLabel: String, onAction: () -> Unit) {
    val isOverdue = statusText.startsWith("OVERDUE")
    Card {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(title, style = MaterialTheme.typography.bodyLarge)
            Text(statusText, color = if (isOverdue) AezelWarning else AezelTextSecondary)
            Spacer(Modifier.height(8.dp))
            Button(onClick = onAction) { Text(actionLabel) }
        }
    }
}

private fun dueStatusText(dueKm: Long, currentKm: Float, unit: String): String {
    if (dueKm <= 0) return "Not yet scheduled"
    val remaining = dueKm - currentKm
    return if (remaining >= 0) "%.0f $unit left".format(remaining) else "OVERDUE by %.0f $unit".format(-remaining)
}

@Composable
private fun DueDateRow(label: String, onDatePicked: (epochSeconds: Long) -> Unit) {
    val context = LocalContext.current
    var pickedLabel by remember { mutableStateOf("Not set") }

    Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        Column(Modifier.weight(1f)) {
            Text(label)
            Text(pickedLabel, color = AezelTextSecondary, style = MaterialTheme.typography.labelSmall)
        }
        Button(onClick = {
            val calendar = Calendar.getInstance()
            DatePickerDialog(
                context,
                { _, year, month, dayOfMonth ->
                    val picked = Calendar.getInstance().apply {
                        set(year, month, dayOfMonth, 0, 0, 0)
                        set(Calendar.MILLISECOND, 0)
                    }
                    val epochSeconds = picked.timeInMillis / 1000
                    pickedLabel = "%04d-%02d-%02d".format(year, month + 1, dayOfMonth)
                    onDatePicked(epochSeconds)
                },
                calendar.get(Calendar.YEAR),
                calendar.get(Calendar.MONTH),
                calendar.get(Calendar.DAY_OF_MONTH),
            ).show()
        }) { Text("Pick Date") }
    }
}
