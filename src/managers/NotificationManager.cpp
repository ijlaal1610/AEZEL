#include "NotificationManager.h"
#include "Config.h"

struct WarningDef { WarningFlag flag; const char* title; NotifPriority prio; };
static const WarningDef kWarningDefs[] = {
    { WarningFlag::CRASH_DETECTED,   "Crash Detected",     NotifPriority::CRITICAL },
    { WarningFlag::ENGINE_OVERTEMP,  "Engine Overheating",  NotifPriority::CRITICAL },
    { WarningFlag::OIL_PRESSURE,      "Oil Pressure Low",    NotifPriority::CRITICAL },
    { WarningFlag::CHARGING_FAULT,    "Charging Fault",      NotifPriority::WARNING },
    { WarningFlag::BATTERY_LOW,        "Battery Low",         NotifPriority::WARNING },
    { WarningFlag::FUEL_LOW,            "Fuel Low",            NotifPriority::WARNING },
    { WarningFlag::ABS_FAULT,            "ABS Fault",           NotifPriority::WARNING },
    { WarningFlag::SD_CARD_FAULT,         "SD Card Error",       NotifPriority::INFO },
    { WarningFlag::GPS_LOST,               "GPS Signal Lost",     NotifPriority::INFO },
    { WarningFlag::SERVICE_DUE,             "Service Due",         NotifPriority::INFO },
    { WarningFlag::TYRE_DUE,                 "Tyre Change Due",     NotifPriority::INFO },
    { WarningFlag::CHAIN_LUBE_DUE,             "Chain Lube Due",      NotifPriority::INFO },
    { WarningFlag::INSURANCE_EXPIRING,          "Insurance Expiring", NotifPriority::INFO },
    { WarningFlag::PUC_EXPIRING,                 "PUC Expiring",       NotifPriority::INFO },
    { WarningFlag::UNAUTHORIZED_MOVE,             "Unauthorized Movement", NotifPriority::CRITICAL },
};

void NotificationManager::begin() {
    pinMode(PIN_OUT_BUZZER, OUTPUT);
}

void NotificationManager::taskEntry(void* pv) {
    NotificationManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(250);
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void NotificationManager::tick() { scanWarningFlags(); }

void NotificationManager::scanWarningFlags() {
    uint32_t mask = SharedState::instance().snapshot().activeWarnings;
    uint32_t newlyRaised = mask & ~_lastWarningMask;
    _lastWarningMask = mask;
    if (!newlyRaised) return;

    for (auto& def : kWarningDefs) {
        if (newlyRaised & uint32_t(def.flag)) {
            push(def.title, "", def.prio);
            soundBuzzerFor(def.prio);
        }
    }
}

void NotificationManager::push(const String& title, const String& body, NotifPriority p) {
    if (_count >= MAX_QUEUE) {
        // Drop the oldest INFO-priority item to make room rather than losing
        // a fresh alert — CRITICAL/WARNING items are never evicted.
        for (size_t i = 0; i < _count; i++) {
            if (_queue[i].priority == NotifPriority::INFO) {
                memmove(&_queue[i], &_queue[i + 1], (_count - i - 1) * sizeof(Notification));
                _count--;
                break;
            }
        }
        if (_count >= MAX_QUEUE) return;   // still full of criticals — genuinely drop
    }
    _queue[_count++] = { title, body, p, millis(), false };
}

const Notification* NotificationManager::current() const {
    // Highest priority, oldest-first, not yet acknowledged
    const Notification* best = nullptr;
    for (size_t i = 0; i < _count; i++) {
        if (_queue[i].acknowledged) continue;
        if (!best || _queue[i].priority > best->priority) best = &_queue[i];
    }
    return best;
}

void NotificationManager::acknowledgeCurrent() {
    for (size_t i = 0; i < _count; i++) {
        if (!_queue[i].acknowledged) { _queue[i].acknowledged = true; return; }
    }
}

void NotificationManager::acknowledgeAt(size_t index) {
    if (index < _count) _queue[index].acknowledged = true;
}

void NotificationManager::soundBuzzerFor(NotifPriority p) {
    if (p == NotifPriority::INFO) return;   // silent for low-priority info items
    int beeps = (p == NotifPriority::CRITICAL) ? 3 : 1;
    for (int i = 0; i < beeps; i++) {
        digitalWrite(PIN_OUT_BUZZER, HIGH);
        delay(80);
        digitalWrite(PIN_OUT_BUZZER, LOW);
        delay(80);
    }
}
