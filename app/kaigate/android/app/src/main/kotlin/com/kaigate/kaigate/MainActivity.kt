package com.kaigate.kaigate

import android.content.Context
import android.media.AudioManager
import android.view.KeyEvent
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {
    private val channel = "kaigate/media"
    private lateinit var sessions: MediaSessionBridge

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        sessions = MediaSessionBridge(this)
        sessions.start()
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, channel)
            .setMethodCallHandler { call, result ->
                val audio = getSystemService(Context.AUDIO_SERVICE) as AudioManager
                when (call.method) {
                    "playPause" -> {
                        pulse(audio, KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE)
                        result.success(null)
                    }
                    "next" -> {
                        pulse(audio, KeyEvent.KEYCODE_MEDIA_NEXT)
                        result.success(null)
                    }
                    "prev" -> {
                        pulse(audio, KeyEvent.KEYCODE_MEDIA_PREVIOUS)
                        result.success(null)
                    }
                    "volUp" -> {
                        audio.adjustStreamVolume(
                            AudioManager.STREAM_MUSIC,
                            AudioManager.ADJUST_RAISE,
                            AudioManager.FLAG_SHOW_UI,
                        )
                        result.success(null)
                    }
                    "volDown" -> {
                        audio.adjustStreamVolume(
                            AudioManager.STREAM_MUSIC,
                            AudioManager.ADJUST_LOWER,
                            AudioManager.FLAG_SHOW_UI,
                        )
                        result.success(null)
                    }
                    "setVolume" -> {
                        val pct = (call.arguments as? Map<*, *>)?.get("volume") as? Int ?: 50
                        val max = audio.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
                        audio.setStreamVolume(
                            AudioManager.STREAM_MUSIC,
                            (max * pct / 100).coerceIn(0, max),
                            AudioManager.FLAG_SHOW_UI,
                        )
                        result.success(null)
                    }
                    "getVolume" -> {
                        val max = audio.getStreamMaxVolume(AudioManager.STREAM_MUSIC).coerceAtLeast(1)
                        val cur = audio.getStreamVolume(AudioManager.STREAM_MUSIC)
                        result.success(cur * 100 / max)
                    }
                    "getNowPlaying" -> result.success(sessions.snapshot())
                    "hasNotificationAccess" -> result.success(sessions.isListenerEnabled())
                    "openNotificationAccess" -> {
                        sessions.openListenerSettings()
                        result.success(null)
                    }
                    else -> result.notImplemented()
                }
            }
    }

    override fun onResume() {
        super.onResume()
        if (::sessions.isInitialized) sessions.start()
    }

    override fun onDestroy() {
        if (::sessions.isInitialized) sessions.stop()
        super.onDestroy()
    }

    private fun pulse(audio: AudioManager, code: Int) {
        audio.dispatchMediaKeyEvent(KeyEvent(KeyEvent.ACTION_DOWN, code))
        audio.dispatchMediaKeyEvent(KeyEvent(KeyEvent.ACTION_UP, code))
    }
}
