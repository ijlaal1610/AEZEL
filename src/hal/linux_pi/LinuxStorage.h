#pragma once
// ============================================================================
//  LinuxStorage.h — Linux file-based configuration and logging storage
// ============================================================================
#include "hal/IHalStorage.h"
#include <map>
#include <string>
#include <mutex>

class LinuxStorage : public IHalStorage {
public:
    explicit LinuxStorage(const std::string& configFilePath = "aezel_config.json");
    virtual ~LinuxStorage() override = default;

    virtual bool begin() override;
    virtual bool setString(const char* key, const std::string& value) override;
    virtual std::string getString(const char* key, const std::string& defaultVal = "") override;
    virtual bool setUInt(const char* key, uint32_t value) override;
    virtual uint32_t getUInt(const char* key, uint32_t defaultVal = 0) override;
    virtual bool setFloat(const char* key, float value) override;
    virtual float getFloat(const char* key, float defaultVal = 0.0f) override;
    virtual bool appendLog(const std::string& path, const std::string& line) override;
    virtual bool isSdMounted() const override;

private:
    void loadFromFile();
    void persistToFile();

    std::string _configFilePath;
    std::mutex _mutex;
    std::map<std::string, std::string> _store;
};
