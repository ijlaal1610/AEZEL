import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../services/ble_service.dart';
import '../theme/aezel_theme.dart';

class AnalyticsScreen extends StatelessWidget {
  const AnalyticsScreen({super.key});

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
            'RIDE ANALYTICS & LOGS',
            style: TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.bold,
              color: AezelColors.primaryCyan,
              letterSpacing: 1.5,
            ),
          ),
          const SizedBox(height: 16),

          GridView.count(
            crossAxisCount: 2,
            crossAxisSpacing: 12,
            mainAxisSpacing: 12,
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            children: [
              _buildTile('MAX SPEED', '${state.maxSpeed} KM/H', Icons.speed, AezelColors.alertRed),
              _buildTile('AVG SPEED', '${state.avgSpeed} KM/H', Icons.av_timer, AezelColors.primaryCyan),
              _buildTile('ODOMETER', '${state.odometer.toStringAsFixed(1)} KM', Icons.directions_bike, AezelColors.neonLime),
              _buildTile('TRIP A', '${state.tripA.toStringAsFixed(1)} KM', Icons.trip_origin, AezelColors.warningAmber),
              _buildTile('TRIP B', '${state.tripB.toStringAsFixed(1)} KM', Icons.alt_route, AezelColors.primaryCyan),
              _buildTile('GPS PIN', '${state.latitude.toStringAsFixed(3)}, ${state.longitude.toStringAsFixed(3)}', Icons.pin_drop, AezelColors.neonLime),
            ],
          ),

          const SizedBox(height: 20),

          // Export Buttons
          Row(
            children: [
              Expanded(
                child: ElevatedButton.icon(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: AezelColors.cardSurface,
                    side: const BorderSide(color: AezelColors.primaryCyan),
                    padding: const EdgeInsets.symmetric(vertical: 14),
                  ),
                  icon: const Icon(Icons.download, color: AezelColors.primaryCyan),
                  label: const Text('Export GPX', style: TextStyle(color: AezelColors.textBright)),
                  onPressed: () {
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text('Exported GPX Track to Files')),
                    );
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ElevatedButton.icon(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: AezelColors.cardSurface,
                    side: const BorderSide(color: AezelColors.neonLime),
                    padding: const EdgeInsets.symmetric(vertical: 14),
                  ),
                  icon: const Icon(Icons.table_chart, color: AezelColors.neonLime),
                  label: const Text('Export CSV', style: TextStyle(color: AezelColors.textBright)),
                  onPressed: () {
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text('Exported CSV Ride Log to Files')),
                    );
                  },
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildTile(String title, String value, IconData icon, Color color) {
    return Container(
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
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                title,
                style: const TextStyle(
                  fontSize: 11,
                  fontWeight: FontWeight.bold,
                  color: AezelColors.textMuted,
                ),
              ),
              Icon(icon, size: 18, color: color),
            ],
          ),
          Text(
            value,
            style: const TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.bold,
              color: AezelColors.textBright,
            ),
          ),
        ],
      ),
    );
  }
}
