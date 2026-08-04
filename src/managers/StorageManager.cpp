#include "StorageManager.h"
#include "Config.h"
#include <SD.h>
#include <SPI.h>
#include <time.h>

void StorageManager::begin() {
    _prefs.begin("phoenix", false);   // read-write namespace

    SPIClass sdSpi(HSPI);
    sdSpi.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    _sdOk = SD.begin(PIN_SD_CS, sdSpi);
    if (!_sdOk) {
        SharedState::instance().raiseWarning(WarningFlag::SD_CARD_FAULT);
    } else {
        if (!SD.exists("/rides")) SD.mkdir("/rides");
        if (!SD.exists("/logs")) SD.mkdir("/logs");
        if (!SD.exists("/export")) SD.mkdir("/export");
    }

    SharedState::instance().update([&](VehicleState& s) { s.sdCardOk = _sdOk; });
    _lastFlushMs = millis();
}

void StorageManager::taskEntry(void* pv) {
    StorageManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(1000);
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void StorageManager::tick() {
    if (millis() - _lastFlushMs > FLUSH_INTERVAL_MS) {
        flushAll();
        _lastFlushMs = millis();
    }
}

void StorageManager::saveOdometer(float km) { _pendingOdometer = km; }
void StorageManager::saveTripA(float km)    { _pendingTripA = km; }
void StorageManager::saveTripB(float km)    { _pendingTripB = km; }

float StorageManager::loadOdometer() { return _prefs.getFloat("odo_km", 0.0f); }
float StorageManager::loadTripA()    { return _prefs.getFloat("tripA_km", 0.0f); }
float StorageManager::loadTripB()    { return _prefs.getFloat("tripB_km", 0.0f); }

void StorageManager::saveMaintenanceRecord(const char* key, uint32_t dueOdometerKm, uint32_t dueEpochSec) {
    char kOdo[24], kTime[24];
    snprintf(kOdo, sizeof(kOdo), "%s_km", key);
    snprintf(kTime, sizeof(kTime), "%s_ts", key);
    _prefs.putUInt(kOdo, dueOdometerKm);
    _prefs.putUInt(kTime, dueEpochSec);
}

void StorageManager::logRidePoint(const RideLogPoint& pt) {
    if (!_sdOk) return;
    char path[48];
    // one CSV file per calendar day keeps files small & export-friendly
    time_t t = pt.timestamp;
    struct tm* tmv = localtime(&t);
    snprintf(path, sizeof(path), "/rides/%04d-%02d-%02d.csv",
             tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday);

    bool isNew = !SD.exists(path);
    File f = SD.open(path, FILE_APPEND);
    if (!f) return;
    if (isNew) f.println("timestamp,lat,lon,speed_kmh,rpm,lean_deg");
    f.printf("%lu,%.6f,%.6f,%.1f,%u,%.1f\n",
              pt.timestamp, pt.lat, pt.lon, pt.speedKmh, pt.rpm, pt.leanAngle);
    f.close();
}

bool StorageManager::exportTripCsv(const char* srcDailyLogPath) {
    if (!_sdOk) return false;
    // Straight copy for now — a richer implementation would merge multiple
    // days for a multi-day trip and recompute trip-relative stats.
    File src = SD.open(srcDailyLogPath, FILE_READ);
    if (!src) return false;
    File dst = SD.open("/export/trip_export.csv", FILE_WRITE);
    if (!dst) { src.close(); return false; }
    while (src.available()) dst.write(src.read());
    src.close();
    dst.close();
    return true;
}

bool StorageManager::exportTripGpx(const char* srcDailyLogPath) {
    if (!_sdOk) return false;
    File src = SD.open(srcDailyLogPath, FILE_READ);
    if (!src) return false;
    File dst = SD.open("/export/trip_export.gpx", FILE_WRITE);
    if (!dst) { src.close(); return false; }

    dst.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    dst.println("<gpx version=\"1.1\" creator=\"ProjectPhoenix\">");
    dst.println("<trk><name>Ride</name><trkseg>");

    src.readStringUntil('\n');   // skip CSV header
    while (src.available()) {
        String line = src.readStringUntil('\n');
        if (line.length() < 5) continue;
        // timestamp,lat,lon,speed,rpm,lean
        int i1 = line.indexOf(','), i2 = line.indexOf(',', i1 + 1), i3 = line.indexOf(',', i2 + 1);
        String lat = line.substring(i1 + 1, i2);
        String lon = line.substring(i2 + 1, i3);
        dst.printf("<trkpt lat=\"%s\" lon=\"%s\"></trkpt>\n", lat.c_str(), lon.c_str());
    }
    dst.println("</trkseg></trk></gpx>");
    src.close();
    dst.close();
    return true;
}

void StorageManager::appendCrashLog(const char* reason) {
    if (!_sdOk) return;
    File f = SD.open("/logs/crash.log", FILE_APPEND);
    if (!f) return;
    f.printf("[%lu] %s\n", millis(), reason);
    f.close();
}

void StorageManager::flushAll() {
    if (_pendingOdometer >= 0) { _prefs.putFloat("odo_km", _pendingOdometer); _pendingOdometer = -1; }
    if (_pendingTripA >= 0)    { _prefs.putFloat("tripA_km", _pendingTripA); _pendingTripA = -1; }
    if (_pendingTripB >= 0)    { _prefs.putFloat("tripB_km", _pendingTripB); _pendingTripB = -1; }
}
