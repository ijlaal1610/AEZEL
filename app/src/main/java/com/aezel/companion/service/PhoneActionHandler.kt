package com.aezel.companion.service

import android.content.Context
import android.content.pm.PackageManager
import android.media.session.MediaController
import android.telecom.TelecomManager
import android.util.Log
import androidx.core.content.ContextCompat

/**
 * Executes a "phone_action" value received in telemetry from the dashboard
 * (see docs/phone_link.md's "What the dashboard sends BACK to the phone"
 * section, and BleManager.cpp's publishTelemetry() on the firmware side).
 *
 * PLATFORM LIMITATION — read before relying on call_reject:
 * Android restricts programmatically REJECTING/ENDING a call to apps that
 * are either the user's default Phone/Dialer app or hold system-level
 * MODIFY_PHONE_STATE permission (not grantable to a normal third-party
 * app). Accepting a ringing call (ANSWER_PHONE_CALLS + TelecomManager) IS
 * available to any app, which is why "call_accept" works below but
 * "call_reject" is honestly a best-effort no-op unless this app is set as
 * the default dialer — which is a heavy ask most users won't want to make
 * just for this feature. The dashboard's Reject button still dismisses the
 * local incoming-call modal either way (see DisplayManager.cpp's
 * onCallRejectClicked, which calls endCall() locally regardless of whether
 * the phone side could actually act on it) — it just may not silence the
 * actual ringing phone.
 */
object PhoneActionHandler {
    private const val TAG = "AezelPhoneAction"

    fun handle(context: Context, action: String) {
        when (action) {
            "call_accept" -> acceptCall(context)
            "call_reject" -> rejectCall(context)
            "music_play_pause" -> togglePlayPause()
            "music_next" -> skipNext()
            else -> Log.w(TAG, "Unknown phone_action: $action")
        }
    }

    private fun acceptCall(context: Context) {
        if (ContextCompat.checkSelfPermission(context, android.Manifest.permission.ANSWER_PHONE_CALLS)
            != PackageManager.PERMISSION_GRANTED
        ) {
            Log.w(TAG, "ANSWER_PHONE_CALLS not granted — cannot accept call programmatically")
            return
        }
        try {
            val telecomManager = context.getSystemService(Context.TELECOM_SERVICE) as? TelecomManager
            telecomManager?.acceptRingingCall()
        } catch (e: SecurityException) {
            Log.w(TAG, "acceptRingingCall failed", e)
        }
    }

    private fun rejectCall(context: Context) {
        // See the class-level PLATFORM LIMITATION note. This attempts the
        // only path a non-default-dialer app has (endCall() via
        // TelecomManager, which silently fails without MODIFY_PHONE_STATE
        // on most devices) rather than pretending this is guaranteed to work.
        try {
            val telecomManager = context.getSystemService(Context.TELECOM_SERVICE) as? TelecomManager
            @Suppress("DEPRECATION")
            telecomManager?.endCall()
        } catch (e: SecurityException) {
            Log.w(TAG, "endCall() unavailable to this app (expected unless set as default dialer)", e)
        }
    }

    private fun togglePlayPause() {
        val controller: MediaController = ActiveMediaSessionHolder.current ?: run {
            Log.w(TAG, "No active media session to control")
            return
        }
        val isPlaying = controller.playbackState?.state == android.media.session.PlaybackState.STATE_PLAYING
        if (isPlaying) controller.transportControls.pause() else controller.transportControls.play()
    }

    private fun skipNext() {
        ActiveMediaSessionHolder.current?.transportControls?.skipToNext()
            ?: Log.w(TAG, "No active media session to skip")
    }
}
