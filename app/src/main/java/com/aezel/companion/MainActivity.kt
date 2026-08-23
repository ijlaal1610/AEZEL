package com.aezel.companion

import android.content.Intent
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Build
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Lock
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.navigation.NavDestination.Companion.hierarchy
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.aezel.companion.ble.BleConnectionState
import com.aezel.companion.data.AezelRepository
import com.aezel.companion.service.AezelForegroundService
import com.aezel.companion.service.PhoneActionHandler
import com.aezel.companion.ui.screens.DashboardScreen
import com.aezel.companion.ui.screens.DeviceScreen
import com.aezel.companion.ui.screens.MaintenanceScreen
import com.aezel.companion.ui.screens.RemoteScreen
import com.aezel.companion.ui.screens.SecurityScreen
import com.aezel.companion.ui.theme.AezelTheme

sealed class AezelDestination(val route: String, val label: String) {
    object Dashboard : AezelDestination("dashboard", "Dashboard")
    object Remote : AezelDestination("remote", "Remote")
    object Security : AezelDestination("security", "Security")
    object Maintenance : AezelDestination("maintenance", "Service")
    object Device : AezelDestination("device", "Device")
}

private val bottomNavDestinations = listOf(
    AezelDestination.Dashboard,
    AezelDestination.Remote,
    AezelDestination.Security,
    AezelDestination.Maintenance,
    AezelDestination.Device,
)

class MainActivity : ComponentActivity() {

    private lateinit var repository: AezelRepository

    private val requestPermissionsLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { /* results observed via ContextCompat.checkSelfPermission where needed */ }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        repository = AezelRepository.getInstance(applicationContext)
        requestRuntimePermissions()

        setContent {
            AezelTheme {
                AezelApp(
                    repository = repository,
                    onOpenNotificationAccessSettings = ::openNotificationAccessSettings,
                    onToggleImmersiveKiosk = ::toggleImmersiveKiosk
                )
            }
        }
    }

    private fun toggleImmersiveKiosk(enable: Boolean) {
        val controller = WindowCompat.getInsetsController(window, window.decorView)
        if (enable) {
            controller.hide(WindowInsetsCompat.Type.systemBars())
            controller.systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        } else {
            controller.show(WindowInsetsCompat.Type.systemBars())
        }
    }

    private fun requestRuntimePermissions() {
        val permissions = mutableListOf<String>()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions += android.Manifest.permission.BLUETOOTH_SCAN
            permissions += android.Manifest.permission.BLUETOOTH_CONNECT
        } else {
            permissions += android.Manifest.permission.ACCESS_FINE_LOCATION
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            permissions += android.Manifest.permission.POST_NOTIFICATIONS
        }
        permissions += android.Manifest.permission.ANSWER_PHONE_CALLS
        requestPermissionsLauncher.launch(permissions.toTypedArray())
    }

    private fun openNotificationAccessSettings() {
        startActivity(Intent("android.settings.ACTION_NOTIFICATION_LISTENER_SETTINGS"))
    }
}

@Composable
fun AezelApp(
    repository: AezelRepository,
    onOpenNotificationAccessSettings: () -> Unit,
    onToggleImmersiveKiosk: (Boolean) -> Unit
) {
    val navController = rememberNavController()
    val vehicleState by repository.lastKnownState.collectAsState()
    val connectionState by repository.connectionState.collectAsState()
    val context = androidx.compose.ui.platform.LocalContext.current
    var isHandlebarCockpitMode by remember { mutableStateOf(false) }

    LaunchedEffect(isHandlebarCockpitMode) {
        onToggleImmersiveKiosk(isHandlebarCockpitMode)
    }

    LaunchedEffect(vehicleState.phoneAction) {
        vehicleState.phoneAction?.let { action ->
            PhoneActionHandler.handle(context, action)
        }
    }

    LaunchedEffect(connectionState) {
        val intent = Intent(context, AezelForegroundService::class.java)
        if (connectionState is BleConnectionState.Connected) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) context.startForegroundService(intent)
            else context.startService(intent)
        } else {
            context.stopService(intent)
        }
    }

    if (isHandlebarCockpitMode) {
        // --- 100% ISOLATED FULL-SCREEN IMMERSIVE COCKPIT DISPLAY FOR GEMINI VCU ---
        DashboardScreen(
            vehicleState = vehicleState,
            connectionState = connectionState,
            bleManager = repository.bleManager,
            isCockpitMode = true,
            onToggleCockpitMode = { isHandlebarCockpitMode = false }
        )
    } else {
        Scaffold(
            bottomBar = {
                val navBackStackEntry by navController.currentBackStackEntryAsState()
                val currentDestination = navBackStackEntry?.destination
                NavigationBar {
                    bottomNavDestinations.forEach { dest ->
                        val selected = currentDestination?.hierarchy?.any { it.route == dest.route } == true
                        NavigationBarItem(
                            selected = selected,
                            onClick = {
                                navController.navigate(dest.route) {
                                    popUpTo(navController.graph.findStartDestination().id) { saveState = true }
                                    launchSingleTop = true
                                    restoreState = true
                                }
                            },
                            icon = { Icon(iconFor(dest), contentDescription = dest.label) },
                            label = { Text(dest.label) },
                        )
                    }
                }
            },
        ) { innerPadding ->
            NavHost(
                navController = navController,
                startDestination = AezelDestination.Dashboard.route,
                modifier = Modifier.padding(innerPadding),
            ) {
                composable(AezelDestination.Dashboard.route) {
                    DashboardScreen(
                        vehicleState = vehicleState,
                        connectionState = connectionState,
                        bleManager = repository.bleManager,
                        isCockpitMode = false,
                        onToggleCockpitMode = { isHandlebarCockpitMode = true }
                    )
                }
                composable(AezelDestination.Remote.route) {
                    RemoteScreen(repository = repository, vehicleState = vehicleState)
                }
                composable(AezelDestination.Security.route) {
                    SecurityScreen(repository = repository, vehicleState = vehicleState)
                }
                composable(AezelDestination.Maintenance.route) {
                    MaintenanceScreen(repository = repository, vehicleState = vehicleState)
                }
                composable(AezelDestination.Device.route) {
                    DeviceScreen(
                        repository = repository,
                        connectionState = connectionState,
                        onOpenNotificationAccessSettings = onOpenNotificationAccessSettings,
                    )
                }
            }
        }
    }
}

private fun iconFor(dest: AezelDestination) = when (dest) {
    AezelDestination.Dashboard -> Icons.Filled.Home
    AezelDestination.Remote -> Icons.Filled.Warning
    AezelDestination.Security -> Icons.Filled.Lock
    AezelDestination.Maintenance -> Icons.Default.Build
    AezelDestination.Device -> Icons.Filled.Settings
}
