#pragma once
// ============================================================================
//  LinuxCan.h — Linux SocketCAN implementation for Raspberry Pi / Linux
// ============================================================================
#include "hal/IHalCan.h"
#include <string>

class LinuxCan : public IHalCan {
public:
    explicit LinuxCan(const std::string& ifName = "can0");
    virtual ~LinuxCan() override;

    virtual bool begin(uint32_t baudRate, int8_t rxPin = -1, int8_t txPin = -1) override;
    virtual bool sendFrame(const CanFrame& frame) override;
    virtual bool receiveFrame(CanFrame& frame, uint32_t timeoutMs = 50) override;
    virtual bool isConnected() const override;
    virtual void stop() override;

private:
    std::string _ifName;
    int _sock = -1;
    bool _connected = false;
};
