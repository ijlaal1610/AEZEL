import 'dart:math';
import 'package:flutter/material.dart';
import '../theme/aezel_theme.dart';

class TachometerGauge extends StatelessWidget {
  final int rpm;
  final int maxRpm;
  final String gear;
  final int speed;
  final bool showSpeedometer;

  const TachometerGauge({
    super.key,
    required this.rpm,
    this.maxRpm = 12000,
    required this.gear,
    required this.speed,
    this.showSpeedometer = true,
  });

  @override
  Widget build(BuildContext meContext) {
    final bool isRedline = rpm >= 9500;

    return AspectRatio(
      aspectRatio: 1.2,
      child: CustomPaint(
        painter: _TachometerPainter(
          rpm: rpm,
          maxRpm: maxRpm,
          isRedline: isRedline,
        ),
        child: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              // Gear Badge
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 18, vertical: 8),
                decoration: BoxDecoration(
                  color: gear == 'N'
                      ? AezelColors.neonLime.withOpacity(0.2)
                      : AezelColors.primaryCyan.withOpacity(0.2),
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(
                    color: gear == 'N' ? AezelColors.neonLime : AezelColors.primaryCyan,
                    width: 2,
                  ),
                ),
                child: Text(
                  gear,
                  style: TextStyle(
                    fontSize: 36,
                    fontWeight: FontWeight.bold,
                    color: gear == 'N' ? AezelColors.neonLime : AezelColors.textBright,
                  ),
                ),
              ),

              if (showSpeedometer) ...[
                const SizedBox(height: 12),
                Text(
                  '$speed',
                  style: TextStyle(
                    fontSize: 64,
                    fontWeight: FontWeight.bold,
                    color: isRedline ? AezelColors.alertRed : AezelColors.textBright,
                    letterSpacing: -2,
                  ),
                ),
                const Text(
                  'KM/H',
                  style: TextStyle(
                    fontSize: 14,
                    fontWeight: FontWeight.w600,
                    color: AezelColors.textMuted,
                    letterSpacing: 2,
                  ),
                ),
              ],
              const SizedBox(height: 8),
              Text(
                '$rpm RPM',
                style: TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.bold,
                  color: isRedline ? AezelColors.alertRed : AezelColors.primaryCyan,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _TachometerPainter extends CustomPainter {
  final int rpm;
  final int maxRpm;
  final bool isRedline;

  _TachometerPainter({
    required this.rpm,
    required this.maxRpm,
    required this.isRedline,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = min(size.width, size.height) * 0.42;

    const startAngle = 135 * (pi / 180);
    const totalSweepAngle = 270 * (pi / 180);

    // Background Track Arc
    final trackPaint = Paint()
      :color = AezelColors.cardBorder
      ..style = PaintingStyle.stroke
      ..strokeWidth = 14
      ..strokeCap = StrokeCap.round;

    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      startAngle,
      totalSweepAngle,
      false,
      trackPaint,
    );

    // Active RPM Sweep Arc
    final progress = (rpm / maxRpm).clamp(0.0, 1.0);
    final activeSweepAngle = totalSweepAngle * progress;

    final activePaint = Paint()
      ..shader = SweepGradient(
        colors: [
          AezelColors.primaryCyan,
          AezelColors.neonLime,
          if (isRedline) AezelColors.alertRed else AezelColors.primaryCyan,
        ],
        stops: const [0.0, 0.7, 1.0],
      ).createShader(Rect.fromCircle(center: center, radius: radius))
      ..style = PaintingStyle.stroke
      ..strokeWidth = 16
      ..strokeCap = StrokeCap.round;

    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      startAngle,
      activeSweepAngle,
      false,
      activePaint,
    );
  }

  @override
  bool shouldRepaint(covariant _TachometerPainter oldDelegate) {
    return oldDelegate.rpm != rpm || oldDelegate.isRedline != isRedline;
  }
}
