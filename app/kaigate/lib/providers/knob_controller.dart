import "dart:async";

import "package:flutter/foundation.dart";
import "package:flutter_blue_plus/flutter_blue_plus.dart";
import "package:shared_preferences/shared_preferences.dart";

import "../ble/kaigate_ble.dart";
import "../ble/protocol.dart";
import "../services/phone_media.dart";

class RunRecord {
  RunRecord(this.at, this.zeroToSixty, this.shiftMs);
  final DateTime at;
  final double zeroToSixty;
  final int shiftMs;
}

class KnobController extends ChangeNotifier {
  KnobController() {
    ble.onTelemetry = (p) {
      telemetry = p;
      _maybeLogBest(p);
      notifyListeners();
    };
    ble.onImu = (s) {
      imu = s;
      notifyListeners();
    };
    ble.onDtc = (j) {
      dtcJson = j;
      notifyListeners();
    };
    ble.onStatus = (j) {
      statusJson = j;
      wifiReady = j["wifi"] == true;
      notifyListeners();
    };
    ble.onMedia = (j) {
      nowPlaying.apply(j);
      final action = j["action"] as String?;
      if (action is String && action.isNotEmpty) {
        PhoneMedia.dispatch(action, volume: nowPlaying.volume);
      }
      notifyListeners();
    };
    ble.onConnection = (c) {
      connected = c;
      notifyListeners();
    };
    _loadHistory();
    _syncPhoneMedia();
    _mediaTimer = Timer.periodic(const Duration(milliseconds: 900), (_) => _syncPhoneMedia());
  }

  final KaiGateBle ble = KaiGateBle();
  TelemetryPacket? telemetry;
  ImuSample? imu;
  KnobConfig config = KnobConfig();
  Map<String, dynamic>? dtcJson;
  Map<String, dynamic>? statusJson;
  bool connected = false;
  bool scanning = false;
  bool wifiReady = false;
  List<ScanResult> devices = [];
  List<RunRecord> history = [];
  NowPlaying nowPlaying = NowPlaying();
  Timer? _mediaTimer;
  String _lastPushedMedia = "";

  Future<void> scan() async {
    scanning = true;
    notifyListeners();
    await ble.startScan();
    ble.scanResults.listen((r) {
      devices = r
          .where((e) =>
              e.device.platformName.contains("KaiGate") ||
              e.advertisementData.serviceUuids
                  .map((u) => u.str.toLowerCase())
                  .contains(KaiGateUuids.service))
          .toList();
      notifyListeners();
    });
    await Future<void>.delayed(const Duration(seconds: 8));
    scanning = false;
    notifyListeners();
  }

  Future<void> connect(BluetoothDevice d) async {
    await ble.connect(d);
    final cfg = await ble.readConfig();
    if (cfg != null) config = cfg;
    await ble.writeNowPlaying(nowPlaying);
    notifyListeners();
  }

  Future<void> disconnect() => ble.disconnect();

  Future<void> applyConfig() async {
    await ble.writeConfig(config);
    notifyListeners();
  }

  Future<void> cmd(String name, [Map<String, dynamic>? extra]) {
    return ble.writeCommand({"cmd": name, ...?extra});
  }

  Future<void> saveGate(int gear) => cmd("saveGate", {"gear": gear});

  Future<void> mediaAction(String action) async {
    if (action == "volUp") nowPlaying.volume = (nowPlaying.volume + 8).clamp(0, 100);
    if (action == "volDown") nowPlaying.volume = (nowPlaying.volume - 8).clamp(0, 100);
    await PhoneMedia.dispatch(action, volume: nowPlaying.volume);
    await _syncPhoneMedia();
  }

  Future<void> openNotificationAccess() => PhoneMedia.openNotificationAccess();

  Future<void> _syncPhoneMedia() async {
    final phone = await PhoneMedia.nowPlaying();
    if (phone == null) return;
    var changed = false;

    if (phone.listenerEnabled != nowPlaying.listenerEnabled) {
      nowPlaying.listenerEnabled = phone.listenerEnabled;
      changed = true;
    }
    if (phone.volume != null && phone.volume != nowPlaying.volume) {
      nowPlaying.volume = phone.volume!;
      changed = true;
    }
    if (phone.playing != nowPlaying.playing) {
      nowPlaying.playing = phone.playing;
      changed = true;
    }

    if (phone.title.isNotEmpty) {
      if (phone.title != nowPlaying.title) {
        nowPlaying.title = phone.title;
        changed = true;
      }
      if (phone.artist != nowPlaying.artist) {
        nowPlaying.artist = phone.artist.isEmpty ? "Phone media" : phone.artist;
        changed = true;
      }
      if (!listEquals(phone.artwork, nowPlaying.artwork)) {
        nowPlaying.artwork = phone.artwork;
        changed = true;
      }
    } else if (phone.listenerEnabled && !phone.playing && nowPlaying.title != "Not playing") {
      nowPlaying.title = "Not playing";
      nowPlaying.artist = "Phone media";
      nowPlaying.artwork = null;
      changed = true;
    }

    if (!changed) return;
    notifyListeners();
    final key = "${nowPlaying.title}|${nowPlaying.artist}|${nowPlaying.playing}|${nowPlaying.volume}";
    if (connected && key != _lastPushedMedia) {
      _lastPushedMedia = key;
      await ble.writeNowPlaying(nowPlaying);
    }
  }

  @override
  void dispose() {
    _mediaTimer?.cancel();
    super.dispose();
  }

  Future<void> setVolume(int volume) async {
    nowPlaying.volume = volume.clamp(0, 100);
    await PhoneMedia.dispatch("setVolume", volume: nowPlaying.volume);
    await ble.writeNowPlaying(nowPlaying);
    notifyListeners();
  }

  Future<void> _loadHistory() async {
    final p = await SharedPreferences.getInstance();
    final raw = p.getStringList("runs") ?? [];
    history = raw.map((e) {
      final p = e.split("|");
      return RunRecord(DateTime.parse(p[0]), double.parse(p[1]), int.parse(p[2]));
    }).toList();
    notifyListeners();
  }

  Future<void> _maybeLogBest(TelemetryPacket p) async {
    if (p.zeroToSixtyS < 2.5) return;
    if (history.isNotEmpty &&
        history.first.zeroToSixty == p.zeroToSixtyS &&
        history.first.shiftMs == p.lastShiftMs) {
      return;
    }
    if (p.speedMph < 59) return;
    history.insert(0, RunRecord(DateTime.now(), p.zeroToSixtyS, p.lastShiftMs));
    if (history.length > 30) history = history.take(30).toList();
    final prefs = await SharedPreferences.getInstance();
    await prefs.setStringList(
      "runs",
      history.map((e) => "${e.at.toIso8601String()}|${e.zeroToSixty}|${e.shiftMs}").toList(),
    );
  }
}
