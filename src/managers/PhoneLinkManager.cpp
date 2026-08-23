#include "PhoneLinkManager.h"

void PhoneLinkManager::showIncomingCall(const String& callerName) {
    update([&](PhoneLinkState& s) {
        s.callActive = true;
        s.callerName = callerName;
        s.callUpdatedMs = millis();
    });
}

void PhoneLinkManager::endCall() {
    update([&](PhoneLinkState& s) {
        s.callActive = false;
        s.callerName = "";
    });
}

void PhoneLinkManager::updateMusic(const String& title, const String& artist, bool playing) {
    update([&](PhoneLinkState& s) {
        s.musicTitle = title;
        s.musicArtist = artist;
        s.musicPlaying = playing;
        s.musicUpdatedMs = millis();
    });
}

void PhoneLinkManager::updateNavigation(const String& instruction, float distanceM) {
    update([&](PhoneLinkState& s) {
        s.navInstruction = instruction;
        s.navDistanceM = distanceM;
        s.navUpdatedMs = millis();
    });
}

void PhoneLinkManager::endNavigation() {
    update([&](PhoneLinkState& s) {
        s.navInstruction = "";
        s.navUpdatedMs = 0;   // 0 = never/ended, matches DisplayPolicyMath::isDataFresh's sentinel
    });
}

void PhoneLinkManager::requestCallAccept()   { update([&](PhoneLinkState& s) { s.pendingPhoneAction = "call_accept"; }); }
void PhoneLinkManager::requestCallReject()   { update([&](PhoneLinkState& s) { s.pendingPhoneAction = "call_reject"; }); }
void PhoneLinkManager::requestMusicPlayPause() { update([&](PhoneLinkState& s) { s.pendingPhoneAction = "music_play_pause"; }); }
void PhoneLinkManager::requestMusicNext()      { update([&](PhoneLinkState& s) { s.pendingPhoneAction = "music_next"; }); }
void PhoneLinkManager::requestMusicPrev()      { update([&](PhoneLinkState& s) { s.pendingPhoneAction = "music_prev"; }); }

String PhoneLinkManager::consumePendingAction() {
    String result;
    update([&](PhoneLinkState& s) {
        result = s.pendingPhoneAction;
        s.pendingPhoneAction = "";
    });
    return result;
}

PhoneLinkState PhoneLinkManager::snapshot() const {
    PhoneLinkState copy;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        copy = _state;
        xSemaphoreGive(_mutex);
    }
    return copy;
}
