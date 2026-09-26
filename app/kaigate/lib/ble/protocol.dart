import "dart:typed_data";

/// BLE contract — keep in lock-step with firmware/include/protocol.h
class KaiGateUuids {
  static const service = "8f400001-7a3b-4c2d-9e1f-0a1b2c3d4e5f";
  static const telemetry = "8f400002-7a3b-4c2d-9e1f-0a1b2c3d4e5f";
  static const config = "8f400003-7a3b-4c2d-9e1f-0a1b2c3d4e5f";
  static const command = "8f400004-7a3b-4c2d-9e1f-0a1b2c3d4e5f";
  static const imu = "8f400005-7a3b-4c2d-9e1f-0a1b2c3d4e5f";
  static const dtc = "8f400006-7a3b-4c2d-9e1f-0a1b2c3d4e5f";
  static const status = "8f400007-7a3b-4c2d-9e1f-0a1b2c3d4e5f";
  static const media = "8f400008-7a3b-4c2d-9e1f-0a1b2c3d4e5f";
}

class TelemetryPacket {
  TelemetryPacket({
    required this.rpm,
    required this.speedMph,
    required this.boostKpa,
    required this.coolantC,
    required this.iatC,
    required this.voltageV,
    required this.gear,
    required this.locked,
    required this.obd,
    required this.strobe,
    required this.moneyShift,
    required this.coldLimited,
    required this.mascot,
    required this.pitch,
    required this.roll,
    required this.latG,
    required this.longG,
    required this.knobBattPct,
    required this.theme,
    required this.lastShiftMs,
    required this.zeroToSixtyS,
  });

  final int rpm;
  final double speedMph;
  final double boostKpa;
  final double coolantC;
  final double iatC;
  final double voltageV;
  final int gear;
  final bool locked;
  final bool obd;
  final bool strobe;
  final bool moneyShift;
  final bool coldLimited;
  final bool mascot;
  final double pitch;
  final double roll;
  final double latG;
  final double longG;
  final int knobBattPct;
  final int theme;
  final int lastShiftMs;
  final double zeroToSixtyS;

  static TelemetryPacket? parse(List<int> d) {
    if (d.length < 28) return null;
    int u16(int i) => d[i] | (d[i + 1] << 8);
    int s16(int i) {
      final v = u16(i);
      return v > 32767 ? v - 65536 : v;
    }

    int s8(int i) {
      final v = d[i];
      return v > 127 ? v - 256 : v;
    }

    final flags = d[13];
    return TelemetryPacket(
      rpm: u16(0),
      speedMph: u16(2) / 10.0,
      boostKpa: s16(4) / 10.0,
      coolantC: s16(6) / 10.0,
      iatC: s16(8) / 10.0,
      voltageV: u16(10) / 1000.0,
      gear: s8(12),
      locked: flags & 0x01 != 0,
      obd: flags & 0x02 != 0,
      strobe: flags & 0x04 != 0,
      moneyShift: flags & 0x08 != 0,
      coldLimited: flags & 0x10 != 0,
      mascot: flags & 0x20 != 0,
      pitch: s16(14) / 100.0,
      roll: s16(16) / 100.0,
      latG: s16(18) / 100.0,
      longG: s16(20) / 100.0,
      knobBattPct: d[22],
      theme: d[23],
      lastShiftMs: u16(24),
      zeroToSixtyS: u16(26) / 100.0,
    );
  }

  String get gearLabel {
    if (gear == -1) return "R";
    if (gear == 0) return "N";
    if (gear >= 1 && gear <= 6) return "$gear";
    return "-";
  }
}

class ImuSample {
  ImuSample(this.pitch, this.roll, this.gx, this.gy, this.gz);
  final double pitch, roll, gx, gy, gz;

  static ImuSample? parse(List<int> d) {
    if (d.length < 10) return null;
    int s16(int i) {
      final v = d[i] | (d[i + 1] << 8);
      return v > 32767 ? v - 65536 : v;
    }

    return ImuSample(s16(0) / 100.0, s16(2) / 100.0, s16(4) / 100.0, s16(6) / 100.0, s16(8) / 100.0);
  }
}

class KnobConfig {
  KnobConfig({
    this.redline = 7200,
    this.shiftLight = 6800,
    this.coldLimit = 3500,
    this.coldCoolantF = 180,
    this.theme = 0,
    this.layout = 1,
    this.reverseLockoutLeft = true,
    this.brightness = 180,
    this.overlay = true,
    this.haptic = true,
    this.audio = true,
  });

  int redline;
  int shiftLight;
  int coldLimit;
  int coldCoolantF;
  int theme;
  int layout;
  bool reverseLockoutLeft;
  int brightness;
  bool overlay;
  bool haptic;
  bool audio;

  Map<String, dynamic> toJson() => {
        "redline": redline,
        "shiftLight": shiftLight,
        "coldLimit": coldLimit,
        "coldCoolantF": coldCoolantF,
        "theme": theme,
        "layout": layout,
        "reverseLockoutLeft": reverseLockoutLeft,
        "brightness": brightness,
        "overlay": overlay,
        "haptic": haptic,
        "audio": audio,
      };

  factory KnobConfig.fromJson(Map<String, dynamic> j) => KnobConfig(
        redline: _asInt(j["redline"], 7200),
        shiftLight: _asInt(j["shiftLight"], 6800),
        coldLimit: _asInt(j["coldLimit"], 3500),
        coldCoolantF: _asInt(j["coldCoolantF"], 180),
        theme: _asInt(j["theme"], 0),
        layout: _asInt(j["layout"], 1),
        reverseLockoutLeft: j["reverseLockoutLeft"] == true || j["reverseLockoutLeft"] == 1,
        brightness: _asInt(j["brightness"], 180),
        overlay: j["overlay"] != false,
        haptic: j["haptic"] != false,
        audio: j["audio"] != false,
      );

  static int _asInt(dynamic v, int d) {
    if (v is int) return v;
    if (v is double) return v.round();
    if (v is String) return int.tryParse(v) ?? d;
    return d;
  }
}

const themeNames = ["JDM Amber", "Euro Sport", "Monochrome", "Cyberpunk Neon"];
const layoutNames = ["5-Speed", "6-Speed", "Dog-leg"];

class NowPlaying {
  NowPlaying({
    this.title = "Not playing",
    this.artist = "Phone media",
    this.playing = false,
    this.volume = 50,
    this.artwork,
    this.listenerEnabled = true,
  });

  String title;
  String artist;
  bool playing;
  int volume;
  Uint8List? artwork;
  bool listenerEnabled;

  Map<String, dynamic> toJson() => {
        "title": title,
        "artist": artist,
        "playing": playing,
        "volume": volume,
      };

  void apply(Map<String, dynamic> j) {
    if (j["title"] is String) title = j["title"] as String;
    if (j["artist"] is String) artist = j["artist"] as String;
    if (j["playing"] is bool) playing = j["playing"] as bool;
    if (j["volume"] is num) volume = (j["volume"] as num).round().clamp(0, 100);
  }
}
