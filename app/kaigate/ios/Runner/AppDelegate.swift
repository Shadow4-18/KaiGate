import AVFoundation
import Flutter
import MediaPlayer
import UIKit

@main
@objc class AppDelegate: FlutterAppDelegate {
  override func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    GeneratedPluginRegistrant.register(with: self)
    if let controller = window?.rootViewController as? FlutterViewController {
      let channel = FlutterMethodChannel(name: "kaigate/media", binaryMessenger: controller.binaryMessenger)
      channel.setMethodCallHandler { call, result in
        let player = MPMusicPlayerController.systemMusicPlayer
        switch call.method {
        case "playPause":
          if player.playbackState == .playing { player.pause() } else { player.play() }
          result(nil)
        case "next":
          player.skipToNextItem()
          result(nil)
        case "prev":
          player.skipToPreviousItem()
          result(nil)
        case "volUp", "volDown", "setVolume":
          result(nil)
        case "getVolume":
          result(Int(AVAudioSession.sharedInstance().outputVolume * 100))
        case "getNowPlaying":
          result(Self.nowPlayingMap(player: player))
        case "hasNotificationAccess":
          result(true)
        case "openNotificationAccess":
          result(nil)
        default:
          result(FlutterMethodNotImplemented)
        }
      }
    }
    return super.application(application, didFinishLaunchingWithOptions: launchOptions)
  }

  private static func nowPlayingMap(player: MPMusicPlayerController) -> [String: Any?] {
    let info = MPNowPlayingInfoCenter.default().nowPlayingInfo
    let item = player.nowPlayingItem
    let title = item?.title
      ?? info?[MPMediaItemPropertyTitle] as? String
      ?? ""
    let artist = item?.artist
      ?? info?[MPMediaItemPropertyArtist] as? String
      ?? ""
    var art: FlutterStandardTypedData?
    if let image = item?.artwork?.image(at: CGSize(width: 512, height: 512)),
       let data = image.pngData() {
      art = FlutterStandardTypedData(bytes: data)
    }
    return [
      "title": title,
      "artist": artist,
      "playing": player.playbackState == .playing,
      "volume": Int(AVAudioSession.sharedInstance().outputVolume * 100),
      "listenerEnabled": true,
      "artwork": art,
    ]
  }
}
