#pragma once
// ============================================================================
//  Esp32Storage.h — ESP32 Preferences & SD MMC implementation
// ============================================================================
#if defined(ESP32) || defined(ARDUINO)
#include "hal/IHalStorage.h"
#include <Preferences.h>
#include <SD.h>

class Esp32Storage : public IHalStorage {
public:
    Esp32Storage() = default;
    virtual ~Esp32Storage() override { _prefs.end(); }

    virtual bool begin() override {
        return _prefs.begin("aezel_hal", false);
    }

    virtual bool setString(const char* key, const std::string& value) override {
        return _prefs.putString(key, value.c_str()) > 0;
    }

    virtual std::string getString(const char* key, const std::string& defaultVal) override {
        String s = _prefs.getString(key, defaultVal.c_str());
        return std::string(s.c_str());
    }

    virtual bool setUInt(const char* key, uint32_t value) override {
        return _prefs.putUInt(key, value) > 0;
    }

    virtual uint32_t getUInt(const char* key, uint32_t defaultVal) override {
        return _prefs.getUInt(key, defaultVal);
    }

    virtual bool setFloat(const char* key, float value) override {
        return _prefs.putFloat(key, value) > 0;
    }

    virtual float getFloat(const char* key, float defaultVal) override {
        return _prefs.getFloat(key, defaultVal);
    }

    virtual bool appendLog(const std::string& path, const std::string& line) override {
        File f = SD.open(path.c_str(), FILE_APPEND);
        if (!f) return false;
        f.println(line.c_str());
        f.close();
        return true;
    }

    virtual bool isSdMounted() const override {
        return SD.cardType() != CARD_NONE;
    }

private:
    Preferences _prefs;
};
#endif
