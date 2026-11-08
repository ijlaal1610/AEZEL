#pragma once
// ============================================================================
//  OtaManager — Over-The-Air (OTA) Wireless Firmware Update Manager
//
//  Launches an embedded Wi-Fi Access Point ("AEZEL-VCU-AP") and HTTP WebServer
//  allowing riders to flash firmware binary updates (.bin) wirelessly from
//  a smartphone or laptop browser without disassembling the motorcycle dashboard.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"

class OtaManager {
public:
    static OtaManager& instance() { static OtaManager o; return o; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    void enableWifiAp();
    void disableWifiAp();
    bool isApActive() const { return _apActive; }

private:
    OtaManager() = default;

    bool _apActive = false;
};
