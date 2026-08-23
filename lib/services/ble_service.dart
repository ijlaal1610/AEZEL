import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../models/vehicle_state.dart';

class BleService extends ChangeNotifier {
  static const String serviceUuid = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
  static const String notifyCharUuid = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
  static const String writeCharUuid = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';

  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _writeCharacteristic;
  StreamSubscription? _notifySubscription;

  bool _isConnected = false;
  bool get isConnected => _isConnected;

  VehicleState _state = VehicleState();
  VehicleState get state => _state;

  String _connectionStatus = 'Disconnected';
  String get connectionStatus => _connectionStatus;

  BleService() {
    _initBle();
  }

  void _initBle() {
    FlutterBluePlus.adapterState.listen((state) {
      if (state == BluetoothAdapterState.on) {
        startScan();
      }
    });
  }

  Future<void> startScan() async {
    _connectionStatus = 'Scanning for AEZEL VCU...';
    notifyListeners();

    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 5));
      FlutterBluePlus.scanResults.listen((results) {
        for (ScanResult r in results) {
          if (r.device.platformName.contains('AEZEL') ||
              r.device.platformName.contains('Phoenix')) {
            FlutterBluePlus.stopScan();
            connectToDevice(r.device);
            break;
          }
        }
      });
    } catch (e) {
      _connectionStatus = 'Scan Error: $e';
      notifyListeners();
    }
  }

  Future<void> connectToDevice(BluetoothDevice device) async {
    _connectionStatus = 'Connecting to ${device.platformName}...';
    notifyListeners();

    try {
      await device.connect(autoConnect: false);
      _connectedDevice = device;
      _isConnected = true;
      _connectionStatus = 'Connected';

      List<BluetoothService> services = await device.discoverServices();
      for (var service in services) {
        if (service.uuid.toString().toLowerCase() == serviceUuid.toLowerCase()) {
          for (var char in service.characteristics) {
            if (char.uuid.toString().toLowerCase() == notifyCharUuid.toLowerCase()) {
              await char.setNotifyValue(true);
              _notifySubscription = char.onValueReceived.listen((value) {
                String jsonStr = utf8.decode(value);
                _state = VehicleState.parseRaw(jsonStr);
                notifyListeners();
              });
            } else if (char.uuid.toString().toLowerCase() == writeCharUuid.toLowerCase()) {
              _writeCharacteristic = char;
            }
          }
        }
      }
    } catch (e) {
      _isConnected = false;
      _connectionStatus = 'Connection Failed';
      notifyListeners();
    }
  }

  Future<void> sendCommand(Map<String, dynamic> commandMap) async {
    if (_writeCharacteristic == null || !_isConnected) return;
    try {
      String jsonCommand = jsonEncode(commandMap);
      await _writeCharacteristic!.write(utf8.encode(jsonCommand), withoutResponse: false);
    } catch (e) {
      debugPrint('BLE Write Error: $e');
    }
  }

  // Remote Control Helpers
  Future<void> remoteEngineStart() => sendCommand({'cmd': 'remote_start_engine'});
  Future<void> toggleIgnition() => sendCommand({'cmd': 'remote_ignition_toggle'});
  Future<void> pulseHorn() => sendCommand({'cmd': 'remote_horn_beep'});
  Future<void> toggleHazard() => sendCommand({'cmd': 'remote_hazard_toggle'});
  Future<void> triggerSeatRelease() => sendCommand({'cmd': 'remote_seat_release'});
  Future<void> triggerFindMyBike() => sendCommand({'cmd': 'find_bike'});
  Future<void> toggleSpeedo() => sendCommand({'cmd': 'toggle_speedo'});
  Future<void> toggleFocus() => sendCommand({'cmd': 'toggle_focus'});
  Future<void> toggleNotifOverlay() => sendCommand({'cmd': 'toggle_notif_overlay'});
  Future<void> toggleLockscreen() => sendCommand({'cmd': 'toggle_lockscreen'});
  Future<void> setPin(String pin) => sendCommand({'cmd': 'set_pin', 'pin': pin});
  Future<void> triggerOta() => sendCommand({'cmd': 'remote_ota_wifi'});

  @override
  void dispose() {
    _notifySubscription?.cancel();
    _connectedDevice?.disconnect();
    super.dispose();
  }
}
