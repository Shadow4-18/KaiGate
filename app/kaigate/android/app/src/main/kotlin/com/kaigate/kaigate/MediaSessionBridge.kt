package com.kaigate.kaigate

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.media.AudioManager
import android.media.MediaMetadata
import android.media.session.MediaController
import android.media.session.MediaSessionManager
import android.media.session.PlaybackState
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import java.io.ByteArrayOutputStream

class MediaSessionBridge(private val context: Context) {
    private val main = Handler(Looper.getMainLooper())
    private val sessions = mutableMapOf<String, MediaController>()
    private val callbacks = mutableMapOf<String, MediaController.Callback>()
    private var cachedArt: ByteArray? = null
    private var cachedArtKey = ""
    private var listening = false

    private val sessionListener =
        MediaSessionManager.OnActiveSessionsChangedListener { controllers ->
            bindControllers(controllers ?: emptyList())
        }

    fun start() {
        if (!isListenerEnabled()) return
        val msm = context.getSystemService(MediaSessionManager::class.java) ?: return
        val cn = component()
        try {
            if (!listening) {
                msm.addOnActiveSessionsChangedListener(sessionListener, cn, main)
                listening = true
            }
            bindControllers(msm.getActiveSessions(cn))
        } catch (_: SecurityException) {
            // User has not granted notification access yet.
        }
    }

    fun stop() {
        val msm = context.getSystemService(MediaSessionManager::class.java)
        if (listening) {
            msm?.removeOnActiveSessionsChangedListener(sessionListener)
            listening = false
        }
        bindControllers(emptyList())
    }

    fun isListenerEnabled(): Boolean {
        val enabled = Settings.Secure.getString(
            context.contentResolver,
            "enabled_notification_listeners",
        ) ?: return false
        val pkg = context.packageName
        return enabled.split(":").any { entry ->
            val cn = ComponentName.unflattenFromString(entry)
            cn?.packageName == pkg || entry.contains(pkg)
        }
    }

    fun openListenerSettings() {
        val intent = Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        context.startActivity(intent)
    }

    fun snapshot(): Map<String, Any?> {
        val audio = context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
        val max = audio.getStreamMaxVolume(AudioManager.STREAM_MUSIC).coerceAtLeast(1)
        val volume = audio.getStreamVolume(AudioManager.STREAM_MUSIC) * 100 / max
        val enabled = isListenerEnabled()
        val controller = preferredController()
        val meta = controller?.metadata
        val state = controller?.playbackState
        val title = meta?.getString(MediaMetadata.METADATA_KEY_TITLE)
            ?: meta?.getString(MediaMetadata.METADATA_KEY_DISPLAY_TITLE)
            ?: ""
        val artist = meta?.getString(MediaMetadata.METADATA_KEY_ARTIST)
            ?: meta?.getString(MediaMetadata.METADATA_KEY_ALBUM_ARTIST)
            ?: meta?.getString(MediaMetadata.METADATA_KEY_DISPLAY_SUBTITLE)
            ?: ""
        val playing = state?.state == PlaybackState.STATE_PLAYING
        val art = artworkBytes(meta, title, artist)
        return hashMapOf(
            "title" to title,
            "artist" to artist,
            "playing" to playing,
            "volume" to volume,
            "listenerEnabled" to enabled,
            "artwork" to art,
        )
    }

    private fun preferredController(): MediaController? {
        val playing = sessions.values.firstOrNull {
            it.playbackState?.state == PlaybackState.STATE_PLAYING
        }
        if (playing != null) return playing
        return sessions.values.firstOrNull { controller ->
            val title = controller.metadata?.getString(MediaMetadata.METADATA_KEY_TITLE)
            !title.isNullOrBlank()
        } ?: sessions.values.firstOrNull()
    }

    private fun bindControllers(controllers: List<MediaController>) {
        val keep = controllers.map { it.sessionToken.toString() }.toSet()
        val stale = sessions.keys.filterNot { keep.contains(it) }
        for (key in stale) {
            sessions.remove(key)?.unregisterCallback(callbacks.remove(key) ?: continue)
        }
        for (controller in controllers) {
            val key = controller.sessionToken.toString()
            if (sessions.containsKey(key)) continue
            val cb = object : MediaController.Callback() {}
            controller.registerCallback(cb, main)
            sessions[key] = controller
            callbacks[key] = cb
        }
    }

    private fun artworkBytes(meta: MediaMetadata?, title: String, artist: String): ByteArray? {
        if (meta == null) return null
        val key = "$title|$artist"
        val bitmap = meta.getBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART)
            ?: meta.getBitmap(MediaMetadata.METADATA_KEY_ART)
            ?: meta.getBitmap(MediaMetadata.METADATA_KEY_DISPLAY_ICON)
        if (bitmap == null) {
            if (key != cachedArtKey) {
                cachedArt = null
                cachedArtKey = key
            }
            return cachedArt
        }
        if (key == cachedArtKey && cachedArt != null) return cachedArt
        val scaled = scale(bitmap, 512)
        val out = ByteArrayOutputStream()
        scaled.compress(Bitmap.CompressFormat.PNG, 90, out)
        cachedArt = out.toByteArray()
        cachedArtKey = key
        return cachedArt
    }

    private fun scale(src: Bitmap, max: Int): Bitmap {
        val longest = maxOf(src.width, src.height)
        if (longest <= max) return src
        val scale = max.toFloat() / longest
        return Bitmap.createScaledBitmap(
            src,
            (src.width * scale).toInt().coerceAtLeast(1),
            (src.height * scale).toInt().coerceAtLeast(1),
            true,
        )
    }

    private fun component() = ComponentName(context, MediaNotificationListener::class.java)
}
