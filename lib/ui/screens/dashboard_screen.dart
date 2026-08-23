import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import '../../services/ble_service.dart';
import '../theme/aezel_theme.dart';
import '../widgets/tachometer_gauge.dart';

class DashboardScreen extends StatefulWidget {
  final VoidCallback onToggleFullscreen;
  final bool isFullscreen;

  const DashboardScreen({
    super.key,
    required this.onToggleFullscreen,
    required this.isFullscreen,
  });

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  @override
  Widget build(BuildContext context) {
    final bleService = Provider.of<BleService>(context);
    final state = bleService.state;

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        children: [
          // Connection & Kiosk Mode Bar
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                decoration: BoxDecoration(
                  color: bleService.isConnected
                      ? AezelColors.neonLime.withOpacity(0.15)
                      : AezelColors.alertRed.withOpacity(0.15),
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(
                    color: bleService.isConnected
                        ? AezelColors.neonLime
                        : AezelColors.alertRed,
                  ),
                ),
                child: Row(
                  children: [
                    Icon(
                      bleService.isConnected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
                      size: 16,
                      color: bleService.isConnected ? AezelColors.neonLime : AezelColors.alertRed,
                    ),
                    const SizedBox(width: 6),
                    Text(
                      bleService.connectionStatus,
                      style: TextStyle(
                        fontSize: 12,
                        fontWeight: FontWeight.w600,
                        color: bleService.isConnected ? AezelColors.neonLime : AezelColors.alertRed,
                      ),
                    ),
                  ],
                ),
              ),

              IconButton(
                icon: Icon(
                  widget.isFullscreen ? Icons.fullscreen_exit : Icons.fullscreen,
                  color: AezelColors.primaryCyan,
                ),
                tooltip: 'Handlebar Cockpit Fullscreen Kiosk Mode',
                onPressed: widget.onToggleFullscreen,
              ),
            ],
          ),

          const SizedBox(height: 16),

          // Central 270° Tachometer & Speedometer
          TachometerGauge(
            rpm: state.rpm,
            gear: state.gear,
            speed: state.speed,
            showSpeedometer: state.showSpeedometer,
          ),

          const SizedBox(height: 20),

          // Active Warnings Grid
          if (state.hasLowFuel ||
              state.hasOverheat ||
              state.hasLowBattery ||
              state.isSideStandDown ||
              state.isTheftAlarmActive)
            _buildWarningBar(state),

          const SizedBox(height: 16),

          // Telemetry Metric Tiles Grid
          GridView.count(
            crossAxisCount: MediaQuery.of(context).orientation == Orientation.landscape ? 4 : 2,
            crossAxisSpacing: 12,
            mainAxisSpacing: 12,
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            children: [
              _buildMetricCard(
                'FUEL RANGE',
                '${state.fuelRange} KM',
                '${state.fuel}% Level',
                Icons.local_gas_station,
                AezelColors.warningAmber,
              ),
              _buildMetricCard(
                'BATTERY',
                '${state.batteryVoltage.toStringAsFixed(1)} V',
                state.batteryVoltage >= 12.4 ? 'Normal Charge' : 'Low Battery',
                Icons.battery_charging_full,
                AezelColors.primaryCyan,
              ),
              _buildMetricCard(
                'ENGINE TEMP',
                '${state.engineTemp} °C',
                state.engineTemp > 105 ? 'OVERHEAT' : 'Normal',
                Icons.thermostat,
                state.engineTemp > 105 ? AezelColors.alertRed : AezelColors.neonLime,
              ),
              _buildMetricCard(
                'LEAN ANGLE',
                '${state.leanAngle.toStringAsFixed(1)}°',
                'Roll Angle',
                Icons.screen_rotation,
                AezelColors.primaryCyan,
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildMetricCard(
    String title,
    String value,
    String subtitle,
    IconData icon,
    Color accentColor,
  ) {
    return Container(
      padding: const EdgeInsets.all(14),
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
                  letterSpacing: 1.2,
                ),
              ),
              Icon(icon, size: 18, color: accentColor),
            ],
          ),
          Text(
            value,
            style: const TextStyle(
              fontSize: 22,
              fontWeight: FontWeight.bold,
              color: AezelColors.textBright,
            ),
          ),
          Text(
            subtitle,
            style: TextStyle(
              fontSize: 11,
              color: accentColor,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildWarningBar(VehicleState state) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: AezelColors.alertRed.withOpacity(0.15),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: AezelColors.alertRed),
      ),
      child: Row(
        children: [
          const Icon(Icons.warning, color: AezelColors.alertRed),
          const SizedBox(width: 12),
          Expanded(
            child: Text(
              _getWarningText(state),
              style: const TextStyle(
                color: AezelColors.alertRed,
                fontWeight: FontWeight.bold,
                fontSize: 13,
              ),
            ),
          ),
        ],
      ),
    );
  }

  String _getWarningText(VehicleState state) {
    List<String> warnings = [];
    if (state.hasLowFuel) warnings.add("LOW FUEL");
    if (state.hasOverheat) warnings.add("ENGINE OVERHEAT");
    if (state.hasLowBattery) warnings.add("LOW BATTERY");
    if (state.isSideStandDown) warnings.add("SIDE STAND DOWN");
    if (state.isTheftAlarmActive) warnings.add("THEFT ALARM");
    return warnings.join(" | ");
  }
}
