import 'dart:convert';

class VehicleState {
  final int speed;
  final int rpm;
  final String gear;
  final int fuel;
  final int fuelRange;
  final double batteryVoltage;
  final int engineTemp;
  final double odometer;
  final double tripA;
  final double tripB;
  final int maxSpeed;
  final int avgSpeed;
  final double leanAngle;
  final bool isIgnitionOn;
  final bool showSpeedometer;
  final bool focusMode;
  final bool allowNotifOverlay;
  final bool enableLockscreen;
  final bool isLocked;
  final int warningMask;
  final double latitude;
  final double longitude;

  VehicleState({
    this.speed = 0,
    this.rpm = 0,
    this.gear = 'N',
    this.fuel = 0,
    this.fuelRange = 0,
    this.batteryVoltage = 12.6,
    this.engineTemp = 25,
    this.odometer = 0.0,
    this.tripA = 0.0,
    this.tripB = 0.0,
    this.maxSpeed = 0,
    this.avgSpeed = 0,
    this.leanAngle = 0.0,
    this.isIgnitionOn = false,
    this.showSpeedometer = true,
    this.focusMode = false,
    this.allowNotifOverlay = true,
    this.enableLockscreen = true,
    this.isLocked = false,
    this.warningMask = 0,
    this.latitude = 0.0,
    this.longitude = 0.0,
  });

  factory VehicleState.fromJson(Map<String, dynamic> json) {
    return VehicleState(
      speed: (json['spd'] as num?)?.toInt() ?? 0,
      rpm: (json['rpm'] as num?)?.toInt() ?? 0,
      gear: (json['gear'] as String?) ?? 'N',
      fuel: (json['fuel'] as num?)?.toInt() ?? 0,
      fuelRange: (json['fuel_rng'] as num?)?.toInt() ?? 0,
      batteryVoltage: (json['batt'] as num?)?.toDouble() ?? 12.6,
      engineTemp: (json['eng_t'] as num?)?.toInt() ?? 25,
      odometer: (json['odo'] as num?)?.toDouble() ?? 0.0,
      tripA: (json['tripA'] as num?)?.toDouble() ?? 0.0,
      tripB: (json['tripB'] as num?)?.toDouble() ?? 0.0,
      maxSpeed: (json['max_spd'] as num?)?.toInt() ?? 0,
      avgSpeed: (json['avg_spd'] as num?)?.toInt() ?? 0,
      leanAngle: (json['lean'] as num?)?.toDouble() ?? 0.0,
      isIgnitionOn: json['ign'] as bool? ?? false,
      showSpeedometer: json['show_spd'] as bool? ?? true,
      focusMode: json['focus'] as bool? ?? false,
      allowNotifOverlay: json['notif_ovl'] as bool? ?? true,
      enableLockscreen: json['lock_en'] as bool? ?? true,
      isLocked: json['locked'] as bool? ?? false,
      warningMask: (json['warn'] as num?)?.toInt() ?? 0,
      latitude: (json['lat'] as num?)?.toDouble() ?? 0.0,
      longitude: (json['lon'] as num?)?.toDouble() ?? 0.0,
    );
  }

  static VehicleState parseRaw(String rawJson) {
    try {
      final Map<String, dynamic> data = jsonDecode(rawJson);
      return VehicleState.fromJson(data);
    } catch (_) {
      return VehicleState();
    }
  }

  // Active Warnings Decoder
  bool get hasLowFuel => (warningMask & (1 << 0)) != 0 || fuel < 15;
  bool get hasOverheat => (warningMask & (1 << 1)) != 0 || engineTemp > 105;
  bool get hasLowBattery => (warningMask & (1 << 2)) != 0 || batteryVoltage < 11.5;
  bool get isSideStandDown => (warningMask & (1 << 3)) != 0;
  bool get isTheftAlarmActive => (warningMask & (1 << 4)) != 0;
}
