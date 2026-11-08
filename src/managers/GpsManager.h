#pragma once
// ============================================================================
//  GpsManager — reads NMEA sentences from a UART GPS module (e.g. NEO-6M/8M)
//  and publishes position/speed/heading/altitude into SharedState. Also
//  disciplines the RTC once a valid fix is available (GPS time is more
//  reliable than a coin-cell RTC that's been sitting in a parts drawer).
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

class GpsManager {
public:
    static GpsManager& instance() { static GpsManager g; return g; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

private:
    GpsManager() = default;
    void syncRtcIfNeeded();
    bool _rtcSynced = false;
};
