#pragma once
// ============================================================================
//  IHalCan.h — Hardware Abstraction Layer for CAN Bus / TWAI
//  Compatible with ESP32-S3 TWAI driver and Raspberry Pi SocketCAN / MCP2515
// ============================================================================
#include <stdint.h>
#include <stddef.h>

struct CanFrame {
    uint32_t id = 0;
    bool     extended = false;
    uint8_t  len = 0;
    uint8_t  data[8] = {0};
};

class IHalCan {
public:
    virtual ~IHalCan() = default;
    virtual bool begin(uint32_t baudRate, int8_t rxPin = -1, int8_t txPin = -1) = 0;
    virtual bool sendFrame(const CanFrame& frame) = 0;
    virtual bool receiveFrame(CanFrame& frame, uint32_t timeoutMs = 50) = 0;
    virtual bool isConnected() const = 0;
    virtual void stop() = 0;
};
