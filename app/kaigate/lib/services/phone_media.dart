import "package:flutter/services.dart";

class PhoneNowPlaying {
  const PhoneNowPlaying({
    this.title = "",
    this.artist = "",
    this.playing = false,
    this.volume,
    this.artwork,
    this.listenerEnabled = true,
  });

  final String title;
  final String artist;
  final bool playing;
  final int? volume;
  final Uint8List? artwork;
  final bool listenerEnabled;
}

class PhoneMedia {
  static const _ch = MethodChannel("kaigate/media");

  static Future<void> dispatch(String action, {int? volume}) async {
    try {
      await _ch.invokeMethod<void>(action, {"volume": volume});
    } on PlatformException {
      // iOS/Android stub or missing engine — UI still syncs to the knob.
    } on MissingPluginException {
      // Platform project not generated yet.
    }
  }

  static Future<int?> volumePercent() async {
    try {
      return await _ch.invokeMethod<int>("getVolume");
    } catch (_) {
      return null;
    }
  }

  static Future<PhoneNowPlaying?> nowPlaying() async {
    try {
      final raw = await _ch.invokeMethod<dynamic>("getNowPlaying");
      if (raw is! Map) return null;
      final map = Map<Object?, Object?>.from(raw);
      final art = map["artwork"];
      return PhoneNowPlaying(
        title: (map["title"] as String?) ?? "",
        artist: (map["artist"] as String?) ?? "",
        playing: map["playing"] == true,
        volume: map["volume"] is num ? (map["volume"] as num).round() : null,
        artwork: art is Uint8List ? art : null,
        listenerEnabled: map["listenerEnabled"] != false,
      );
    } catch (_) {
      return null;
    }
  }

  static Future<bool> hasNotificationAccess() async {
    try {
      return await _ch.invokeMethod<bool>("hasNotificationAccess") ?? false;
    } catch (_) {
      return true;
    }
  }

  static Future<void> openNotificationAccess() async {
    try {
      await _ch.invokeMethod<void>("openNotificationAccess");
    } catch (_) {}
  }
}
