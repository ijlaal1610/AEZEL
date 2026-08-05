package com.aezel.vcu.service

import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import com.aezel.vcu.ble.AezelBleManager
import com.google.gson.JsonObject

/**
 * Android NotificationListenerService that captures incoming phone notifications
 * (Calls, WhatsApp, SMS, Navigation) and mirrors them to the AEZEL Cockpit Display via BLE.
 */
class AezelNotificationListenerService : NotificationListenerService() {

    private var lastNotifHash: Int = 0
    private var lastNotifTime: Long = 0

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        super.onNotificationPosted(sbn)
        if (sbn == null) return

        val packageName = sbn.packageName ?: return
        val extras = sbn.notification.extras ?: return

        val title = extras.getCharSequence("android.title")?.toString() ?: return
        val text = extras.getCharSequence("android.text")?.toString() ?: ""

        // Filter system noise / persistent ongoing notifications
        if (title.isBlank() || sbn.isOngoing) return

        // Throttle duplicate notifications within 2 seconds
        val currentHash = (packageName + title + text).hashCode()
        val currentTime = System.currentTimeMillis()
        if (currentHash == lastNotifHash && (currentTime - lastNotifTime) < 2000) return
        lastNotifHash = currentHash
        lastNotifTime = currentTime

        val appLabel = when {
            packageName.contains("whatsapp") -> "WhatsApp"
            packageName.contains("telecom") || packageName.contains("dialer") || packageName.contains("incall") -> "Incoming Call"
            packageName.contains("messaging") || packageName.contains("mms") || packageName.contains("sms") -> "SMS"
            packageName.contains("instagram") -> "Instagram"
            packageName.contains("maps") -> "Google Maps"
            else -> packageName.substringAfterLast('.').capitalize()
        }

        // Construct GATT JSON Payload
        val payload = JsonObject().apply {
            addProperty("cmd", "phone_notif")
            addProperty("app", appLabel)
            addProperty("title", title)
            addProperty("body", text.take(64)) // Truncate long bodies to save BLE bandwidth
        }.toString()

        // Send to AEZEL Cockpit via BLE
        try {
            AezelBleManager.instance.sendCommand(payload)
        } catch (e: Exception) {
            // BLE not initialized / disconnected
        }
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification?) {
        super.onNotificationRemoved(sbn)
    }
}
