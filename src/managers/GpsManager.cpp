#include "GpsManager.h"
#include "Config.h"
#include <TinyGPS++.h>
#include <RTClib.h>

static TinyGPSPlus gps;
static HardwareSerial gpsSerial(GPS_UART_NUM);
static RTC_DS3231 rtc;
static bool s_rtcOk = false;
static uint32_t s_bootMs = 0;

void GpsManager::begin() {
    s_bootMs = millis();
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    s_rtcOk = rtc.begin();

    SharedState::instance().update([&](VehicleState& s) {
        s.gpsModuleOk = false; // set to true once bytes are decoded
    });
}

void GpsManager::taskEntry(void* pv) {
    GpsManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(200);   // 5 Hz poll
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void GpsManager::tick() {
    bool newFix = false;
    while (gpsSerial.available() > 0) {
        if (gps.encode(gpsSerial.read())) newFix = true;
    }

    // Module connectivity check: did we process NMEA sentences?
    bool moduleAlive = (gps.charsProcessed() > 10);
    SharedState::instance().update([=](VehicleState& s) { s.gpsModuleOk = moduleAlive; });

    // Warning handling: flag lost if no fix after 30s or age > 5000ms
    bool fixValid = gps.location.isValid() && (gps.location.age() < 5000);
    if (!fixValid) {
        if (millis() - s_bootMs > 30000) {
            SharedState::instance().raiseWarning(WarningFlag::GPS_LOST);
        }
    } else {
        SharedState::instance().clearWarning(WarningFlag::GPS_LOST);
    }

    if (!newFix) return;

    SharedState::instance().update([&](VehicleState& s) {
        s.gpsFixValid = fixValid;
        if (fixValid) {
            s.latitude = gps.location.lat();
            s.longitude = gps.location.lng();
        }
        if (gps.speed.isValid()) s.gpsSpeedKmh = gps.speed.kmph();
        if (gps.altitude.isValid()) s.altitudeM = gps.altitude.meters();
        if (gps.course.isValid()) s.headingDeg = gps.course.deg();
        if (gps.satellites.isValid()) s.gpsSatellites = gps.satellites.value();
    });

    syncRtcIfNeeded();
}

void GpsManager::syncRtcIfNeeded() {
    if (_rtcSynced || !s_rtcOk || !gps.date.isValid() || !gps.time.isValid()) return;
    if (gps.date.year() < 2024) return;   // sanity check

    rtc.adjust(DateTime(gps.date.year(), gps.date.month(), gps.date.day(),
                          gps.time.hour(), gps.time.minute(), gps.time.second()));
    _rtcSynced = true;
}
