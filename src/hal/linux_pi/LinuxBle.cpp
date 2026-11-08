#include "LinuxBle.h"
#include <iostream>

bool LinuxBle::begin(const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(_mutex);
    _deviceName = deviceName;
    _connected = false;
    return true;
}

bool LinuxBle::isConnected() const {
    return _connected;
}

bool LinuxBle::notifyTelemetry(const uint8_t* data, size_t length) {
    (void)data;
    (void)length;
    // In Linux companion mode, packets can be pushed via socket or BlueZ characteristic
    return _connected;
}

void LinuxBle::onCommandReceived(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(_mutex);
    _cmdCallback = callback;
}

void LinuxBle::stop() {
    std::lock_guard<std::mutex> lock(_mutex);
    _connected = false;
}

void LinuxBle::simulateIncomingCommand(const std::string& cmd) {
    std::function<void(const std::string&)> cb;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        cb = _cmdCallback;
    }
    if (cb) {
        cb(cmd);
    }
}

void LinuxBle::setConnected(bool conn) {
    std::lock_guard<std::mutex> lock(_mutex);
    _connected = conn;
}
