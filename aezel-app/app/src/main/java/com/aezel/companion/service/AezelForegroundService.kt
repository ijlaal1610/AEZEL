package com.aezel.companion.service

import android.app.Notification
import android.app.Service
import android.content.Intent
import android.os.IBinder
import androidx.core.app.NotificationCompat
import com.aezel.companion.AezelApplication
import com.aezel.companion.MainActivity
import com.aezel.companion.R

/**
 * A plain foreground service whose only job is to keep the process (and
 * therefore the BLE GATT connection + NotificationForwardingService) alive
 * while the app is backgrounded — e.g. phone in a pocket/tank bag during
 * an actual ride, screen off. Without this, Android's background execution
 * limits would eventually kill the BLE connection and the phone-link
 * features (calls/messages/music/nav — see docs/phone_link.md on the
 * firmware side) would silently stop working mid-ride.
 *
 * Started from MainActivity once a BLE connection is established, stopped
 * on explicit disconnect. Does not itself own the BLE connection — that
 * lives in AezelRepository (a process-wide singleton, unaffected by this
 * service's lifecycle either way) — this only prevents the OS from killing
 * the process the repository lives in.
 */
class AezelForegroundService : Service() {

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        startForeground(NOTIFICATION_ID, buildNotification())
        return START_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun buildNotification(): Notification {
        val openAppIntent = Intent(this, MainActivity::class.java)
        val pendingIntent = android.app.PendingIntent.getActivity(
            this, 0, openAppIntent,
            android.app.PendingIntent.FLAG_IMMUTABLE,
        )

        return NotificationCompat.Builder(this, AezelApplication.CHANNEL_CONNECTION)
            .setContentTitle(getString(R.string.fg_service_notification_title))
            .setContentText(getString(R.string.fg_service_notification_text))
            .setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)   // placeholder system icon — swap for a real AEZEL glyph, see docs/build.md
            .setOngoing(true)
            .setContentIntent(pendingIntent)
            .build()
    }

    companion object {
        private const val NOTIFICATION_ID = 1001
    }
}
