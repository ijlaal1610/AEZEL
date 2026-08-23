#pragma once
// ============================================================================
//  PhoneLinkManager — holds whatever the companion app has pushed over BLE:
//  an active call, recent messages (routed into NotificationManager, not
//  duplicated here), current music track, and the next-turn navigation hint.
//  DisplayManager reads it every frame; BleManager writes into it from
//  handleIncomingCommand() and reads pending outbound actions (accept/
//  reject a call, play/pause/skip music) to include in telemetry.
//
//  IMPORTANT — what this is and isn't: this firmware has no way to read
//  your phone's actual calls/SMS/WhatsApp/music itself. All of that has to
//  come FROM a companion app that has the relevant OS permissions
//  (Android's Notification Listener Service, media session APIs, etc.) and
//  forwards the data over the BLE command channel below. That companion
//  app is a separate project, not covered by this firmware — see
//  docs/phone_link.md for the exact JSON contract it needs to speak.
//
//  Thread-safety follows the same pattern as SharedState in DataModel.h:
//  a small mutex-guarded struct, snapshot-to-read / update-under-lock-to-write.
// ============================================================================
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct PhoneLinkState {
    // --- Incoming call ---
    bool callActive = false;
    String callerName;
    uint32_t callUpdatedMs = 0;

    // --- Now playing ---
    String musicTitle;
    String musicArtist;
    bool musicPlaying = false;
    uint32_t musicUpdatedMs = 0;

    // --- Navigation relay (next-turn hint from the phone's own nav app) ---
    String navInstruction;
    float navDistanceM = 0;
    uint32_t navUpdatedMs = 0;

    // --- One-shot outbound action for BleManager to pick up and clear ---
    String pendingPhoneAction;
};

class PhoneLinkManager {
public:
    static PhoneLinkManager& instance() { static PhoneLinkManager p; return p; }

    // --- Inbound, called from BleManager::handleIncomingCommand() ---
    void showIncomingCall(const String& callerName);
    void endCall();
    void updateMusic(const String& title, const String& artist, bool playing);
    void updateNavigation(const String& instruction, float distanceM);
    void endNavigation();

    // --- Outbound requests, called from DisplayManager button handlers ---
    void requestCallAccept();
    void requestCallReject();
    void requestMusicPlayPause();
    void requestMusicNext();
    void requestMusicPrev();

    // Returns the pending action (if any) and clears it — call exactly
    // once per BLE telemetry publish so it's delivered once, not resent
    // on every 500ms telemetry tick.
    String consumePendingAction();

    // --- Read access for DisplayManager, staleness-checked internally ---
    PhoneLinkState snapshot() const;

private:
    PhoneLinkManager() { _mutex = xSemaphoreCreateMutex(); }
    template <typename Fn>
    void update(Fn&& fn) {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            fn(_state);
            xSemaphoreGive(_mutex);
        }
    }

    PhoneLinkState _state;
    mutable SemaphoreHandle_t _mutex;
};
