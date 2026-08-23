import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';

import 'services/ble_service.dart';
import 'ui/theme/aezel_theme.dart';
import 'ui/widgets/quick_drawer.dart';
import 'ui/screens/dashboard_screen.dart';
import 'ui/screens/remote_control_screen.dart';
import 'ui/screens/navigation_screen.dart';
import 'ui/screens/analytics_screen.dart';
import 'ui/screens/settings_screen.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(
    ChangeNotifierProvider(
      create: (_) => BleService(),
      child: const AezelApp(),
    ),
  );
}

class AezelApp extends StatelessWidget {
  const AezelApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'AEZEL Cockpit',
      debugShowCheckedModeBanner: false,
      theme: AezelTheme.darkTheme,
      home: const MainContainerScreen(),
    );
  }
}

class MainContainerScreen extends StatefulWidget {
  const MainContainerScreen({super.key});

  @override
  State<MainContainerScreen> createState() => _MainContainerScreenState();
}

class _MainContainerScreenState extends State<MainContainerScreen> {
  int _currentIndex = 0;
  bool _isFullscreenKiosk = false;

  void _toggleFullscreenKiosk() {
    setState(() {
      _isFullscreenKiosk = !_isFullscreenKiosk;
      if (_isFullscreenKiosk) {
        SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersiveSticky);
      } else {
        SystemChrome.setEnabledSystemUIMode(SystemUiMode.edgeToEdge);
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    final List<Widget> screens = [
      DashboardScreen(
        onToggleFullscreen: _toggleFullscreenKiosk,
        isFullscreen: _isFullscreenKiosk,
      ),
      const RemoteControlScreen(),
      const NavigationScreen(),
      const AnalyticsScreen(),
      const SettingsScreen(),
    ];

    final List<String> titles = [
      'MAIN COCKPIT',
      'REMOTE CONTROLS',
      'GPS NAVIGATION',
      'RIDE ANALYTICS',
      'SETTINGS & SECURITY',
    ];

    return Scaffold(
      appBar: _isFullscreenKiosk
          ? null
          : AppBar(
              title: Text(
                titles[_currentIndex],
                style: const TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 1.5,
                  color: AezelColors.primaryCyan,
                ),
              ),
              actions: [
                IconButton(
                  icon: const Icon(Icons.bluetooth_searching),
                  onPressed: () {
                    Provider.of<BleService>(context, listen: false).startScan();
                  },
                ),
              ],
            ),
      drawer: _isFullscreenKiosk
          ? null
          : QuickDrawer(
              selectedIndex: _currentIndex,
              onItemSelected: (index) {
                setState(() {
                  _currentIndex = index;
                });
              },
            ),
      body: GestureDetector(
        onHorizontalDragEnd: (details) {
          if (details.primaryVelocity != null) {
            if (details.primaryVelocity! < -300) {
              // Swipe Left -> Next Screen
              if (_currentIndex < screens.length - 1) {
                setState(() => _currentIndex++);
              }
            } else if (details.primaryVelocity! > 300) {
              // Swipe Right -> Previous Screen
              if (_currentIndex > 0) {
                setState(() => _currentIndex--);
              }
            }
          }
        },
        child: AnimatedSwitcher(
          duration: const Duration(milliseconds: 300),
          child: screens[_currentIndex],
        ),
      ),
      bottomNavigationBar: _isFullscreenKiosk
          ? null
          : BottomNavigationBar(
              currentIndex: _currentIndex,
              type: BottomNavigationBarType.fixed,
              backgroundColor: AezelColors.cardSurface,
              selectedItemColor: AezelColors.primaryCyan,
              unselectedItemColor: AezelColors.textMuted,
              onTap: (index) {
                setState(() {
                  _currentIndex = index;
                });
              },
              items: const [
                BottomNavigationBarItem(
                  icon: Icon(Icons.speed),
                  label: 'Cockpit',
                ),
                BottomNavigationBarItem(
                  icon: Icon(Icons.tune),
                  label: 'Controls',
                ),
                BottomNavigationBarItem(
                  icon: Icon(Icons.navigation),
                  label: 'Nav',
                ),
                BottomNavigationBarItem(
                  icon: Icon(Icons.analytics),
                  label: 'Analytics',
                ),
                BottomNavigationBarItem(
                  icon: Icon(Icons.settings),
                  label: 'Settings',
                ),
              ],
            ),
    );
  }
}
