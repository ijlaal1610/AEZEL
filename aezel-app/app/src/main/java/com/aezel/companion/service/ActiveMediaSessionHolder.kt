package com.aezel.companion.service

import android.media.session.MediaController

/**
 * Simple process-wide holder for whichever MediaController
 * NotificationForwardingService's session listener last saw as active —
 * PhoneActionHandler reads this to actually execute play/pause/skip when a
 * "music_play_pause"/"music_next" phone_action arrives from the dashboard.
 * Not persisted, not thread-synchronized beyond @Volatile — a stale/null
 * read just means a button press does nothing rather than crashing, which
 * is the right failure mode for "the phone's music app changed state
 * between the dashboard button press and this being read."
 */
object ActiveMediaSessionHolder {
    @Volatile
    var current: MediaController? = null
}
