#pragma once
// ============================================================================
//  IHalBle.h — Hardware Abstraction Layer for Bluetooth Low Energy
//  Compatible with ESP32-S3 (NimBLE-Arduino) and Raspberry Pi (BlueZ / Mock BLE)
// ============================================================================
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <functional>

class IHalBle {
public:
    virtual ~IHalBle() = default;
    virtual bool begin(const std::string& deviceName) = 0;
    virtual bool isConnected() const = 0;
    virtual bool notifyTelemetry(const uint8_t* data, size_t length) = 0;
    virtual void onCommandReceived(std::function<void(const std::string&)> callback) = 0;
    virtual void stop() = 0;
};
