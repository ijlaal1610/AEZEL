import 'package:flutter/material.dart';

class AezelColors {
  static const Color backgroundDark = Color(0xFF07090E);
  static const Color cardSurface = Color(0xFF10141E);
  static const Color cardBorder = Color(0xFF1E2638);

  static const Color primaryCyan = Color(0xFF00D4FF);
  static const Color alertRed = Color(0xFFFF1744);
  static const Color neonLime = Color(0xFF39FF14);
  static const Color warningAmber = Color(0xFFFFB300);
  static const Color textMuted = Color(0xFF8A99AD);
  static const Color textBright = Color(0xFFF0F4F8);
}

class AezelTheme {
  static ThemeData get darkTheme {
    return ThemeData.dark().copyWith(
      scaffoldBackgroundColor: AezelColors.backgroundDark,
      primaryColor: AezelColors.primaryCyan,
      cardColor: AezelColors.cardSurface,
      colorScheme: const ColorScheme.dark(
        primary: AezelColors.primaryCyan,
        secondary: AezelColors.neonLime,
        error: AezelColors.alertRed,
        surface: AezelColors.cardSurface,
      ),
      appBarTheme: const AppBarTheme(
        backgroundColor: AezelColors.backgroundDark,
        elevation: 0,
        centerTitle: true,
      ),
    );
  }
}
