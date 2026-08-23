package com.aezel.companion.service

import android.app.Notification
import android.content.ComponentName
import android.media.MediaMetadata
import android.media.session.MediaSessionManager
import android.media.session.PlaybackState
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import com.aezel.companion.data.AezelRepository

/**
 * Requires the user to manually grant "Notification Access" in Android
 * Settings — this cannot be requested via a normal runtime permission
 * dialog (see MainActivity's settings-deeplink button for where that grant
 * flow is triggered from). Once granted, this service can see the title/
 * text of every notification posted system-wide, which is exactly the
 * sensitive capability the call/message-mirror feature needs and exactly
 * why Android gates it behind a manual grant rather than a tappable
 * permission prompt.
 *
 * ACCURACY NOTE — read this before trusting any of the parsing below:
 * Android does not provide a clean, stable API for "give me the caller's
 * name" or "give me this app's message preview" — this service reads
 * whatever a given app chose to put in its notification's title/text
 * fields, which varies by app and can change on any app update without
 * warning. The call-detection path (Notification.CATEGORY_CALL) is a
 * documented, stable Android API and is reasonably reliable. The
 * message-app package matching and the Maps navigation text parsing below
 * are both best-effort heuristics — see docs/phone_link.md's caveats
 * section, and expect to need to adjust the package/text-format constants
 * below if a message app or Maps changes its notification format.
 */
class NotificationForwardingService : NotificationListenerService() {

    private var mediaSessionManager: MediaSessionManager? = null
    private val mediaSessionListener = MediaSessionManager.OnActiveSessionsChangedListener { controllers ->
        val active = controllers?.firstOrNull { it.playbackState?.state == PlaybackState.STATE_PLAYING }
            ?: controllers?.firstOrNull()
        ActiveMediaSessionHolder.current = active
        val metadata = active?.metadata ?: return@OnActiveSessionsChangedListener
        val title = metadata.getString(MediaMetadata.METADATA_KEY_TITLE) ?: return@OnActiveSessionsChangedListener
        val artist = metadata.getString(MediaMetadata.METADATA_KEY_ARTIST) ?: ""
        val playing = active.playbackState?.state == PlaybackState.STATE_PLAYING
        repository()?.pushMusicUpdate(title, artist, playing)
    }

    override fun onListenerConnected() {
        super.onListenerConnected()
        Log.i(TAG, "Notification access granted, listener connected")
        // Notification-listener access is also what grants permission to
        // enumerate active media sessions — this is standard Android
        // behavior, not an AEZEL-specific design choice.
        mediaSessionManager = getSystemService(MediaSessionManager::class.java)
        val componentName = ComponentName(this, NotificationForwardingService::class.java)
        try {
            mediaSessionManager?.addOnActiveSessionsChangedListener(mediaSessionListener, componentName)
            // addOnActiveSessionsChangedListener only fires on FUTURE changes —
            // sessions already active before this service connected need an
            // explicit initial fetch, or a song already playing when the app
            // starts won't show up until the next track change.
            mediaSessionListener.onActiveSessionsChanged(mediaSessionManager?.getActiveSessions(componentName))
        } catch (e: SecurityException) {
            Log.w(TAG, "Media session listener registration failed", e)
        }
    }

    override fun onListenerDisconnected() {
        super.onListenerDisconnected()
        mediaSessionManager?.removeOnActiveSessionsChangedListener(mediaSessionListener)
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        val notification = sbn.notification ?: return
        val extras = notification.extras
        val title = extras.getCharSequence(Notification.EXTRA_TITLE)?.toString() ?: ""
        val text = extras.getCharSequence(Notification.EXTRA_TEXT)?.toString() ?: ""

        when {
            // Stable, documented Android API — reliable across devices/OEMs.
            notification.category == Notification.CATEGORY_CALL -> {
                if (title.isNotBlank()) repository()?.pushIncomingCall(title)
            }

            // Heuristic: known messaging app packages. Add more here as
            // needed — see the ACCURACY NOTE above.
            sbn.packageName in KNOWN_MESSAGE_APPS -> {
                if (title.isNotBlank() || text.isNotBlank()) {
                    val appLabel = KNOWN_MESSAGE_APPS[sbn.packageName] ?: sbn.packageName
                    repository()?.pushMessage(appLabel, title, text)
                }
            }

            // Heuristic: Google Maps' ongoing navigation notification.
            // EXTRA_TEXT typically looks like "500 m · Turn right onto Main
            // St" but this format is entirely Maps' internal choice and not
            // a public contract — see docs/phone_link.md.
            sbn.packageName == PACKAGE_GOOGLE_MAPS -> {
                parseMapsNavigationText(text)?.let { (instruction, distanceM) ->
                    repository()?.pushNavUpdate(instruction, distanceM)
                }
            }
        }
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        if (sbn.notification?.category == Notification.CATEGORY_CALL) {
            repository()?.pushCallEnded()
        }
        if (sbn.packageName == PACKAGE_GOOGLE_MAPS) {
            repository()?.pushNavEnd()
        }
    }

    private fun repository(): AezelRepository? = try {
        AezelRepository.getInstance(applicationContext)
    } catch (e: Exception) {
        null
    }

    /**
     * Best-effort parse of "<distance> · <instruction>" style text.
     * Returns null (rather than a guess) if the format doesn't match what's
     * expected — a missed nav update is far better than a wrong one shown
     * on the dashboard.
     */
    private fun parseMapsNavigationText(text: String): Pair<String, Float>? {
        val parts = text.split("·", "-").map { it.trim() }
        if (parts.size < 2) return null

        val distancePart = parts[0]
        val instruction = parts.drop(1).joinToString(" ").trim()
        if (instruction.isBlank()) return null

        val distanceM = parseDistanceToMeters(distancePart) ?: return null
        return instruction to distanceM
    }

    private fun parseDistanceToMeters(text: String): Float? {
        val match = Regex("([0-9.]+)\\s*(m|km|ft|mi)").find(text.lowercase()) ?: return null
        val value = match.groupValues[1].toFloatOrNull() ?: return null
        return when (match.groupValues[2]) {
            "km" -> value * 1000f
            "mi" -> value * 1609.34f
            "ft" -> value * 0.3048f
            else -> value   // "m"
        }
    }

    companion object {
        private const val TAG = "AezelNotifForward"
        private const val PACKAGE_GOOGLE_MAPS = "com.google.android.apps.maps"

        // package name -> human-readable app label used in push_message's "app" field.
        private val KNOWN_MESSAGE_APPS = mapOf(
            "com.whatsapp" to "WhatsApp",
            "org.telegram.messenger" to "Telegram",
            "com.google.android.apps.messaging" to "Messages",
            "com.samsung.android.messaging" to "Messages",
        )
    }
}
