package com.aezel.vcu

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import com.aezel.vcu.ble.AezelBleManager
import com.aezel.vcu.ui.screens.AnalyticsScreen
import com.aezel.vcu.ui.screens.DashboardScreen
import com.aezel.vcu.ui.screens.NavigationScreen
import com.aezel.vcu.ui.screens.RemoteControlScreen
import com.aezel.vcu.ui.theme.AEZELTheme
import com.aezel.vcu.ui.theme.AccentCyan
import com.aezel.vcu.ui.theme.CardBorderDark
import com.aezel.vcu.ui.theme.CardSurfaceDark

class MainActivity : ComponentActivity() {

    private lateinit var bleManager: AezelBleManager

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        if (permissions.values.all { it }) {
            bleManager.startScan()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        bleManager = AezelBleManager.getInstance(this)

        checkAndRequestPermissions()

        setContent {
            AEZELTheme {
                MainAppStructure(bleManager)
            }
        }
    }

    private fun checkAndRequestPermissions() {
        val permissions = mutableListOf(
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.ACCESS_COARSE_LOCATION
        )

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions.add(Manifest.permission.BLUETOOTH_SCAN)
            permissions.add(Manifest.permission.BLUETOOTH_CONNECT)
        }

        val missing = permissions.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }

        if (missing.isNotEmpty()) {
            permissionLauncher.launch(missing.toTypedArray())
        } else {
            bleManager.startScan()
        }
    }
}

enum class NavigationTab(val title: String, val icon: ImageVector) {
    DASHBOARD("Dashboard", Icons.Default.Speed),
    REMOTE("Remote", Icons.Default.VpnKey),
    NAV("Navigation", Icons.Default.Navigation),
    ANALYTICS("Analytics", Icons.Default.Analytics)
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainAppStructure(bleManager: AezelBleManager) {
    var selectedTab by remember { mutableStateOf(NavigationTab.DASHBOARD) }
    val telemetry by bleManager.telemetry.collectAsState()
    val connState by bleManager.connectionState.collectAsState()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("AEZEL SMART VCU", color = AccentCyan) },
                actions = {
                    IconButton(onClick = {
                        if (telemetry.bleConnected) bleManager.disconnect() else bleManager.startScan()
                    }) {
                        Icon(
                            imageVector = if (telemetry.bleConnected) Icons.Default.BluetoothConnected else Icons.Default.BluetoothSearching,
                            contentDescription = "BLE Connection",
                            tint = if (telemetry.bleConnected) AccentCyan else MaterialTheme.colorScheme.tertiary
                        )
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = CardSurfaceDark)
            )
        },
        bottomBar = {
            NavigationBar(
                containerColor = CardSurfaceDark,
                contentColor = AccentCyan
            ) {
                NavigationTab.values().forEach { tab ->
                    NavigationBarItem(
                        selected = selectedTab == tab,
                        onClick = { selectedTab = tab },
                        icon = { Icon(tab.icon, contentDescription = tab.title) },
                        label = { Text(tab.title) }
                    )
                }
            }
        }
    ) { innerPadding ->
        Box(modifier = Modifier.padding(innerPadding)) {
            when (selectedTab) {
                NavigationTab.DASHBOARD -> DashboardScreen(telemetry)
                NavigationTab.REMOTE -> RemoteControlScreen(bleManager, telemetry)
                NavigationTab.NAV -> NavigationScreen(bleManager)
                NavigationTab.ANALYTICS -> AnalyticsScreen(telemetry)
            }
        }
    }
}
