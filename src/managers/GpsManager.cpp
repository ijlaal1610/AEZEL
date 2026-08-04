#include "GpsManager.h"
#include "Config.h"
#include <TinyGPS++.h>
#include <RTClib.h>

static TinyGPSPlus gps;
static HardwareSerial gpsSerial(GPS_UART_NUM);
static RTC_DS3231 rtc;

void GpsManager::begin() {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    bool rtcOk = rtc.begin();
    SharedState::instance().update([&](VehicleState& s) { s.gpsModuleOk = true; (void)rtcOk; });
}

void GpsManager::taskEntry(void* pv) {
    GpsManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(200);   // 5 Hz poll; GPS itself usually outputs at 1Hz
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

    if (!newFix) {
        // Flag GPS as lost if we haven't had a valid sentence in a while
        if (gps.location.age() > 5000 && gps.location.isValid()) {
            SharedState::instance().raiseWarning(WarningFlag::GPS_LOST);
        }
        return;
    }
    SharedState::instance().clearWarning(WarningFlag::GPS_LOST);

    SharedState::instance().update([&](VehicleState& s) {
        s.gpsFixValid = gps.location.isValid();
        if (gps.location.isValid()) {
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
    if (_rtcSynced || !gps.date.isValid() || !gps.time.isValid()) return;
    if (gps.date.year() < 2024) return;   // sanity check against a not-yet-locked fix

    rtc.adjust(DateTime(gps.date.year(), gps.date.month(), gps.date.day(),
                          gps.time.hour(), gps.time.minute(), gps.time.second()));
    _rtcSynced = true;   // once per boot is enough; re-sync daily is a cheap future improvement
}
