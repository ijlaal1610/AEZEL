#pragma once
// ============================================================================
//  LinuxBle.h — Linux / Raspberry Pi BLE implementation
// ============================================================================
#include "hal/IHalBle.h"
#include <mutex>
#include <vector>

class LinuxBle : public IHalBle {
public:
    LinuxBle() = default;
    virtual ~LinuxBle() override = default;

    virtual bool begin(const std::string& deviceName) override;
    virtual bool isConnected() const override;
    virtual bool notifyTelemetry(const uint8_t* data, size_t length) override;
    virtual void onCommandReceived(std::function<void(const std::string&)> callback) override;
    virtual void stop() override;

    // Helper for simulation / testing on Linux
    void simulateIncomingCommand(const std::string& cmd);
    void setConnected(bool conn);

private:
    std::string _deviceName;
    bool _connected = false;
    std::function<void(const std::string&)> _cmdCallback = nullptr;
    std::mutex _mutex;
};
