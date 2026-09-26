import "dart:async";
import "dart:convert";
import "dart:io";

import "package:flutter_blue_plus/flutter_blue_plus.dart";
import "package:permission_handler/permission_handler.dart";

import "protocol.dart";

typedef TelemetryFn = void Function(TelemetryPacket pkt);
typedef ImuFn = void Function(ImuSample imu);
typedef JsonFn = void Function(Map<String, dynamic> json);

class KaiGateBle {
  BluetoothDevice? device;
  BluetoothCharacteristic? _tel, _cfg, _cmd, _imu, _dtc, _status, _media;
  StreamSubscription<List<int>>? _telSub, _imuSub, _dtcSub, _stSub, _mediaSub;
  StreamSubscription<List<ScanResult>>? _scanSub;

  TelemetryFn? onTelemetry;
  ImuFn? onImu;
  JsonFn? onDtc;
  JsonFn? onStatus;
  JsonFn? onMedia;
  void Function(bool)? onConnection;

  bool get isConnected => device != null && (_tel != null);

  Future<void> _ensurePermissions() async {
    if (Platform.isAndroid) {
      await [
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.locationWhenInUse,
      ].request();
    }
  }

  Future<void> startScan({Duration timeout = const Duration(seconds: 8)}) async {
    await _ensurePermissions();
    await FlutterBluePlus.adapterState.where((s) => s == BluetoothAdapterState.on).first;
    await FlutterBluePlus.stopScan();
    await FlutterBluePlus.startScan(
      withServices: [Guid(KaiGateUuids.service)],
      timeout: timeout,
    );
  }

  Stream<List<ScanResult>> get scanResults => FlutterBluePlus.scanResults;

  Future<void> connect(BluetoothDevice d) async {
    await FlutterBluePlus.stopScan();
    device = d;
    await d.connect(timeout: const Duration(seconds: 12));
    await d.discoverServices();
    for (final s in d.servicesList) {
      if (s.uuid.str.toLowerCase() != KaiGateUuids.service) continue;
      for (final c in s.characteristics) {
        final id = c.uuid.str.toLowerCase();
        if (id == KaiGateUuids.telemetry) _tel = c;
        if (id == KaiGateUuids.config) _cfg = c;
        if (id == KaiGateUuids.command) _cmd = c;
        if (id == KaiGateUuids.imu) _imu = c;
        if (id == KaiGateUuids.dtc) _dtc = c;
        if (id == KaiGateUuids.status) _status = c;
        if (id == KaiGateUuids.media) _media = c;
      }
    }
    if (_tel != null) {
      await _tel!.setNotifyValue(true);
      _telSub = _tel!.onValueReceived.listen((v) {
        final p = TelemetryPacket.parse(v);
        if (p != null) onTelemetry?.call(p);
      });
    }
    if (_imu != null) {
      await _imu!.setNotifyValue(true);
      _imuSub = _imu!.onValueReceived.listen((v) {
        final p = ImuSample.parse(v);
        if (p != null) onImu?.call(p);
      });
    }
    if (_dtc != null) {
      await _dtc!.setNotifyValue(true);
      _dtcSub = _dtc!.onValueReceived.listen((v) {
        onDtc?.call(jsonDecode(utf8.decode(v)) as Map<String, dynamic>);
      });
    }
    if (_status != null) {
      await _status!.setNotifyValue(true);
      _stSub = _status!.onValueReceived.listen((v) {
        onStatus?.call(jsonDecode(utf8.decode(v)) as Map<String, dynamic>);
      });
    }
    if (_media != null) {
      await _media!.setNotifyValue(true);
      _mediaSub = _media!.onValueReceived.listen((v) {
        onMedia?.call(jsonDecode(utf8.decode(v)) as Map<String, dynamic>);
      });
    }
    onConnection?.call(true);
  }

  Future<void> disconnect() async {
    await _telSub?.cancel();
    await _imuSub?.cancel();
    await _dtcSub?.cancel();
    await _stSub?.cancel();
    await _mediaSub?.cancel();
    await _scanSub?.cancel();
    _tel = _cfg = _cmd = _imu = _dtc = _status = _media = null;
    await device?.disconnect();
    device = null;
    onConnection?.call(false);
  }

  Future<void> writeCommand(Map<String, dynamic> cmd) async {
    if (_cmd == null) return;
    await _cmd!.write(utf8.encode(jsonEncode(cmd)), withoutResponse: true);
  }

  Future<void> writeConfig(KnobConfig cfg) async {
    if (_cfg == null) return;
    await _cfg!.write(utf8.encode(jsonEncode(cfg.toJson())), withoutResponse: false);
  }

  Future<KnobConfig?> readConfig() async {
    if (_cfg == null) return null;
    final v = await _cfg!.read();
    return KnobConfig.fromJson(jsonDecode(utf8.decode(v)) as Map<String, dynamic>);
  }

  Future<void> writeNowPlaying(NowPlaying np) async {
    if (_media == null) return;
    await _media!.write(utf8.encode(jsonEncode(np.toJson())), withoutResponse: true);
  }
}
