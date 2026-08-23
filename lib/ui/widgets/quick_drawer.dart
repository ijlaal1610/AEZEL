import 'package:flutter/material.dart';
import '../theme/aezel_theme.dart';

class QuickDrawer extends StatelessWidget {
  final int selectedIndex;
  final Function(int) onItemSelected;

  const QuickDrawer({
    super.key,
    required this.selectedIndex,
    required this.onItemSelected,
  });

  @override
  Widget build(BuildContext context) {
    return Drawer(
      backgroundColor: AezelColors.cardSurface,
      child: SafeArea(
        child: Column(
          children: [
            Container(
              padding: const EdgeInsets.all(24),
              width: double.infinity,
              decoration: const BoxDecoration(
                border: Border(bottom: BorderSide(color: AezelColors.cardBorder)),
              ),
              child: const Column(
                crossAxisAlignment: CrossAlignment.start,
                children: [
                  Text(
                    'AEZEL VCU',
                    style: TextStyle(
                      fontSize: 22,
                      fontWeight: FontWeight.bold,
                      color: AezelColors.primaryCyan,
                      letterSpacing: 2,
                    ),
                  ),
                  SizedBox(height: 4),
                  Text(
                    'Cross-Platform Cockpit',
                    style: TextStyle(
                      fontSize: 12,
                      color: AezelColors.textMuted,
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 12),
            _buildDrawerTile(context, 0, Icons.speed, 'Main Cockpit'),
            _buildDrawerTile(context, 1, Icons.tune, 'Remote Controls'),
            _buildDrawerTile(context, 2, Icons.navigation, 'GPS Navigation'),
            _buildDrawerTile(context, 3, Icons.analytics, 'Ride Analytics'),
            _buildDrawerTile(context, 4, Icons.settings, 'Settings'),
          ],
        ),
      ),
    );
  }

  Widget _buildDrawerTile(
    BuildContext context,
    int index,
    IconData icon,
    String title,
  ) {
    final isSelected = selectedIndex == index;

    return ListTile(
      leading: Icon(
        icon,
        color: isSelected ? AezelColors.primaryCyan : AezelColors.textMuted,
      ),
      title: Text(
        title,
        style: TextStyle(
          color: isSelected ? AezelColors.textBright : AezelColors.textMuted,
          fontWeight: isSelected ? FontWeight.bold : FontWeight.normal,
        ),
      ),
      selected: isSelected,
      selectedTileColor: AezelColors.primaryCyan.withOpacity(0.1),
      onTap: () {
        Navigator.pop(context);
        onItemSelected(index);
      },
    );
  }
}
