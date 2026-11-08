#pragma once
// ============================================================================
//  HalProvider.h — Central Registry for Active Platform HAL Implementations
// ============================================================================
#include "IHalGpio.h"
#include "IHalCan.h"
#include "IHalBle.h"
#include "IHalStorage.h"

class HalProvider {
public:
    static HalProvider& instance() {
        static HalProvider s_instance;
        return s_instance;
    }

    void setGpio(IHalGpio* gpio) { _gpio = gpio; }
    void setCan(IHalCan* can) { _can = can; }
    void setBle(IHalBle* ble) { _ble = ble; }
    void setStorage(IHalStorage* storage) { _storage = storage; }

    static IHalGpio* gpio() { return instance()._gpio; }
    static IHalCan* can() { return instance()._can; }
    static IHalBle* ble() { return instance()._ble; }
    static IHalStorage* storage() { return instance()._storage; }

private:
    HalProvider() = default;
    IHalGpio* _gpio = nullptr;
    IHalCan* _can = nullptr;
    IHalBle* _ble = nullptr;
    IHalStorage* _storage = nullptr;
};
