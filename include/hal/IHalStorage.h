#pragma once
// ============================================================================
//  IHalStorage.h — Hardware Abstraction Layer for Non-Volatile & SD Storage
//  Compatible with ESP32-S3 (Preferences/SD_MMC) and Raspberry Pi (POSIX JSON/Filesystem)
// ============================================================================
#include <stdint.h>
#include <stddef.h>
#include <string>

class IHalStorage {
public:
    virtual ~IHalStorage() = default;
    virtual bool begin() = 0;
    virtual bool setString(const char* key, const std::string& value) = 0;
    virtual std::string getString(const char* key, const std::string& defaultVal = "") = 0;
    virtual bool setUInt(const char* key, uint32_t value) = 0;
    virtual uint32_t getUInt(const char* key, uint32_t defaultVal = 0) = 0;
    virtual bool setFloat(const char* key, float value) = 0;
    virtual float getFloat(const char* key, float defaultVal = 0.0f) = 0;
    virtual bool appendLog(const std::string& path, const std::string& line) = 0;
    virtual bool isSdMounted() const = 0;
};
