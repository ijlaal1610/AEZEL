import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../services/ble_service.dart';
import '../theme/aezel_theme.dart';

class SettingsScreen extends StatelessWidget {
  const SettingsScreen({super.key});

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
            'COCKPIT PREFERENCES & SECURITY',
            style: TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.bold,
              color: AezelColors.primaryCyan,
              letterSpacing: 1.5,
            ),
          ),
          const SizedBox(height: 16),

          _buildSwitchTile(
            'Show Numeric Speedometer',
            'Show or hide center speed readout',
            state.showSpeedometer,
            (val) => bleService.toggleSpeedo(),
          ),
          _buildSwitchTile(
            'Minimalist Focus Mode',
            'Hide non-essential widgets during performance riding',
            state.focusMode,
            (val) => bleService.toggleFocus(),
          ),
          _buildSwitchTile(
            'Live Riding Notification Overlay',
            'Pop up phone calls / WhatsApp on main display',
            state.allowNotifOverlay,
            (val) => bleService.toggleNotifOverlay(),
          ),
          _buildSwitchTile(
            'Anti-Theft Security Lockscreen',
            'Require 4-digit PIN or BLE proximity auto-unlock',
            state.enableLockscreen,
            (val) => bleService.toggleLockscreen(),
          ),

          const SizedBox(height: 20),

          // Security PIN Setup Button
          ListTile(
            tileColor: AezelColors.cardSurface,
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(16),
              side: const BorderSide(color: AezelColors.cardBorder),
            ),
            leading: const Icon(Icons.password, color: AezelColors.primaryCyan),
            title: const Text('Change Security PIN Code', style: TextStyle(color: AezelColors.textBright)),
            subtitle: const Text('Configure 4-digit immobilizer PIN', style: TextStyle(color: AezelColors.textMuted)),
            trailing: const Icon(Icons.chevron_right, color: AezelColors.textMuted),
            onTap: () {
              _showPinDialog(context, bleService);
            },
          ),
        ],
      ),
    );
  }

  Widget _buildSwitchTile(
    String title,
    String subtitle,
    bool value,
    Function(bool) onChanged,
  ) {
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      decoration: BoxDecoration(
        color: AezelColors.cardSurface,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: AezelColors.cardBorder),
      ),
      child: SwitchListTile(
        title: Text(title, style: const TextStyle(color: AezelColors.textBright, fontWeight: FontWeight.bold)),
        subtitle: Text(subtitle, style: const TextStyle(color: AezelColors.textMuted, fontSize: 12)),
        value: value,
        activeColor: AezelColors.primaryCyan,
        onChanged: onChanged,
      ),
    );
  }

  void _showPinDialog(BuildContext context, BleService bleService) {
    final controller = TextEditingController();
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: AezelColors.cardSurface,
        title: const Text('Set 4-Digit Security PIN'),
        content: TextField(
          controller: controller,
          keyboardType: TextInputType.number,
          maxLength: 4,
          style: const TextStyle(color: AezelColors.textBright, fontSize: 24, letterSpacing: 8),
          decoration: const InputDecoration(
            hintText: '1234',
            hintStyle: TextStyle(color: AezelColors.textMuted),
          ),
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              if (controller.text.length == 4) {
                bleService.setPin(controller.text);
                Navigator.pop(ctx);
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(content: Text('PIN Updated to ${controller.text}')),
                );
              }
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
  }
}
