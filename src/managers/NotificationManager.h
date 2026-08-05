#pragma once
// ============================================================================
//  NotificationManager — turns raw WarningFlag bits + phone notifications
//  (call/SMS/WhatsApp forwarded over BLE, future work) into a prioritized,
//  de-duplicated queue that DisplayManager renders as toasts/icons and that
//  drives the buzzer for anything safety-critical.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

enum class NotifPriority : uint8_t { INFO, WARNING, CRITICAL };

struct Notification {
    String title;
    String body;
    NotifPriority priority;
    uint32_t createdMs;
    bool acknowledged = false;
};

class NotificationManager {
public:
    static NotificationManager& instance() { static NotificationManager n; return n; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    void push(const String& title, const String& body, NotifPriority p);
    const Notification* current() const;   // top unacknowledged notification, or nullptr
    void acknowledgeCurrent();

    // Read access for the Notifications screen's list view.
    size_t count() const { return _count; }
    const Notification& at(size_t index) const { return _queue[index]; }
    void acknowledgeAt(size_t index);

private:
    NotificationManager() = default;
    void scanWarningFlags();
    void soundBuzzerFor(NotifPriority p);

    static constexpr size_t MAX_QUEUE = 16;
    Notification _queue[MAX_QUEUE];
    size_t _count = 0;
    uint32_t _lastWarningMask = 0;
};
