import 'package:flutter/material.dart';
import '../theme/aezel_theme.dart';

class NavigationScreen extends StatefulWidget {
  const NavigationScreen({super.key});

  @override
  State<NavigationScreen> createState() => _NavigationScreenState();
}

class _NavigationScreenState extends State<NavigationScreen> {
  bool _isAutoMirrorEnabled = true;

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAlignment.start,
        children: [
          const Text(
            'GPS NAVIGATION BRIDGE',
            style: TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.bold,
              color: AezelColors.primaryCyan,
              letterSpacing: 1.5,
            ),
          ),
          const SizedBox(height: 16),

          // Automated Bridge Status Card
          Container(
            padding: const EdgeInsets.all(20),
            decoration: BoxDecoration(
              color: AezelColors.cardSurface,
              borderRadius: BorderRadius.circular(16),
              border: Border.all(color: AezelColors.cardBorder),
            ),
            child: Column(
              crossAxisAlignment: CrossAlignment.start,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text(
                      'Automated Maps Mirroring',
                      style: TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                        color: AezelColors.textBright,
                      ),
                    ),
                    Switch(
                      value: _isAutoMirrorEnabled,
                      activeColor: AezelColors.primaryCyan,
                      onChanged: (val) {
                        setState(() {
                          _isAutoMirrorEnabled = val;
                        });
                      },
                    ),
                  ],
                ),
                const SizedBox(height: 8),
                const Text(
                  'When enabled, turn directions from Google Maps or Apple Maps running on your phone are automatically streamed over BLE to your motorcycle display. Zero manual buttons required while riding.',
                  style: TextStyle(
                    fontSize: 12,
                    color: AezelColors.textMuted,
                    height: 1.4,
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: 20),

          // Active Turn Preview Box
          Container(
            width: double.infinity,
            padding: const EdgeInsets.all(24),
            decoration: BoxDecoration(
              color: AezelColors.cardSurface,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: AezelColors.primaryCyan),
            ),
            child: Column(
              children: [
                const Icon(
                  Icons.turn_left,
                  size: 64,
                  color: AezelColors.primaryCyan,
                ),
                const SizedBox(height: 12),
                const Text(
                  'In 150 meters',
                  style: TextStyle(
                    fontSize: 24,
                    fontWeight: FontWeight.bold,
                    color: AezelColors.textBright,
                  ),
                ),
                const SizedBox(height: 4),
                const Text(
                  'Turn Left onto MG Road / NH-44',
                  style: TextStyle(
                    fontSize: 14,
                    color: AezelColors.textMuted,
                  ),
                ),
                const SizedBox(height: 16),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                  decoration: BoxDecoration(
                    color: AezelColors.primaryCyan.withOpacity(0.15),
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: const Text(
                    'ETA: 12 mins | 4.8 km remaining',
                    style: TextStyle(
                      fontSize: 12,
                      fontWeight: FontWeight.w600,
                      color: AezelColors.primaryCyan,
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
