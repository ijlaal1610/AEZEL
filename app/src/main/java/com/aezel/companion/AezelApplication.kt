package com.aezel.companion

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.os.Build

class AezelApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        createNotificationChannels()
    }

    private fun createNotificationChannels() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return
        val manager = getSystemService(NotificationManager::class.java)
        val channel = NotificationChannel(
            CHANNEL_CONNECTION,
            getString(R.string.notification_channel_connection),
            NotificationManager.IMPORTANCE_LOW,   // low importance — this is a persistent status, not an alert
        ).apply {
            description = getString(R.string.notification_channel_connection_desc)
        }
        manager.createNotificationChannel(channel)
    }

    companion object {
        const val CHANNEL_CONNECTION = "aezel_connection"
    }
}
