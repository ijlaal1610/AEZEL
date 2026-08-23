import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../services/ble_service.dart';
import '../theme/aezel_theme.dart';

class RemoteControlScreen extends StatelessWidget {
  const RemoteControlScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final bleService = Provider.of<BleService>(context);
    final state = bleService.state;

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAlignment.start,
        children: [
          const Text(
            'ACTUATORS & REMOTE CONTROL',
            style: TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.bold,
              color: AezelColors.primaryCyan,
              letterSpacing: 1.5,
            ),
          ),
          const SizedBox(height: 16),

          // Prominent Remote Engine Start Button
          Container(
            width: double.infinity,
            padding: const EdgeInsets.all(20),
            decoration: BoxDecoration(
              gradient: LinearGradient(
                colors: [
                  AezelColors.primaryCyan.withOpacity(0.2),
                  AezelColors.cardSurface,
                ],
              ),
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: AezelColors.primaryCyan, width: 2),
            ),
            child: Column(
              children: [
                const Icon(
                  Icons.power_settings_new,
                  size: 48,
                  color: AezelColors.primaryCyan,
                ),
                const SizedBox(height: 12),
                const Text(
                  'REMOTE ENGINE START',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                    color: AezelColors.textBright,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  state.gear == 'N'
                      ? '✓ Safety Interlock Ready (Gear is Neutral N)'
                      : '⚠️ WARNING: Shift to Neutral (N) to start',
                  style: TextStyle(
                    fontSize: 12,
                    color: state.gear == 'N'
                        ? AezelColors.neonLime
                        : AezelColors.warningAmber,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                const SizedBox(height: 16),
                ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: state.gear == 'N'
                        ? AezelColors.primaryCyan
                        : AezelColors.cardBorder,
                    padding: const EdgeInsets.symmetric(
                        horizontal: 32, vertical: 14),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12),
                    ),
                  ),
                  onPressed: state.gear == 'N'
                      ? () {
                          bleService.remoteEngineStart();
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(
                              content: Text('⚡ Remote Engine Start Dispatched!'),
                            ),
                          );
                        }
                      : () {
                          showDialog(
                            context: context,
                            builder: (ctx) => AlertDialog(
                              backgroundColor: AezelColors.cardSurface,
                              title: const Text('Safety Interlock Violation'),
                              content: const Text(
                                'Remote engine start is blocked unless the motorcycle is in Neutral (N) gear.',
                              ),
                              actions: [
                                TextButton(
                                  onPressed: () => Navigator.pop(ctx),
                                  child: const Text('OK'),
                                ),
                              ],
                            ),
                          );
                        },
                  child: const Text(
                    '🚀 START ENGINE',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                      color: Colors.black,
                    ),
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: 20),

          // Control Tile Grid
          GridView.count(
            crossAxisCount: 2,
            crossAxisSpacing: 12,
            mainAxisSpacing: 12,
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            children: [
              _buildTile(
                'Keyless Ignition',
                state.isIgnitionOn ? 'POWER ON' : 'POWER OFF',
                Icons.vibration,
                state.isIgnitionOn ? AezelColors.neonLime : AezelColors.textMuted,
                () => bleService.toggleIgnition(),
              ),
              _buildTile(
                'Horn Pulse',
                '300ms Pulse',
                Icons.volume_up,
                AezelColors.primaryCyan,
                () => bleService.pulseHorn(),
              ),
              _buildTile(
                'Hazard Lights',
                'Dual Flasher',
                Icons.warning_amber,
                AezelColors.warningAmber,
                () => bleService.toggleHazard(),
              ),
              _buildTile(
                'Unlock Seat',
                '500ms Solenoid',
                Icons.lock_open,
                AezelColors.neonLime,
                () => bleService.triggerSeatRelease(),
              ),
              _buildTile(
                'Find My Bike',
                'Panic Alarm',
                Icons.location_searching,
                AezelColors.alertRed,
                () => bleService.triggerFindMyBike(),
              ),
              _buildTile(
                'Firmware OTA',
                'Start Wi-Fi AP',
                Icons.wifi_protected_setup,
                AezelColors.primaryCyan,
                () => bleService.triggerOta(),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildTile(
    String title,
    String subtitle,
    IconData icon,
    Color color,
    VoidCallback onTap,
  ) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(16),
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: AezelColors.cardSurface,
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: AezelColors.cardBorder),
        ),
        child: Column(
          crossAxisAlignment: CrossAlignment.start,
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Icon(icon, size: 28, color: color),
            Column(
              crossAxisAlignment: CrossAlignment.start,
              children: [
                Text(
                  title,
                  style: const TextStyle(
                    fontSize: 14,
                    fontWeight: FontWeight.bold,
                    color: AezelColors.textBright,
                  ),
                ),
                Text(
                  subtitle,
                  style: TextStyle(
                    fontSize: 11,
                    color: color,
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
