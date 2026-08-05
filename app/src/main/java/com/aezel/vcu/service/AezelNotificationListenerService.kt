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

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        super.onNotificationPosted(sbn)
        if (sbn == null) return

        val packageName = sbn.packageName ?: return
        val extras = sbn.notification.extras ?: return

        val title = extras.getCharSequence("android.title")?.toString() ?: return
        val text = extras.getCharSequence("android.text")?.toString() ?: ""

        // Filter system noise / persistent ongoing notifications
        if (title.isBlank() || sbn.isOngoing) return

        val appLabel = when {
            packageName.contains("whatsapp") -> "WhatsApp"
            packageName.contains("telecom") || packageName.contains("dialer") -> "Incoming Call"
            packageName.contains("messaging") || packageName.contains("mms") -> "SMS"
            packageName.contains("instagram") -> "Instagram"
            packageName.contains("maps") -> "Google Maps"
            else -> packageName.substringAfterLast('.')
        }

        // Construct GATT JSON Payload
        val payload = JsonObject().apply {
            addProperty("cmd", "phone_notif")
            addProperty("app", appLabel)
            addProperty("title", title)
            addProperty("body", text)
        }.toString()

        // Send to AEZEL Cockpit via BLE
        AezelBleManager.instance.sendCommand(payload)
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification?) {
        super.onNotificationRemoved(sbn)
    }
}
