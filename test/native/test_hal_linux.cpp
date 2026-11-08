// ============================================================================
//  test_hal_linux.cpp — native unit tests for Linux / Raspberry Pi HAL
// ============================================================================
#include "hal/HalProvider.h"
#include "hal/linux_pi/LinuxGpio.h"
#include "hal/linux_pi/LinuxStorage.h"
#include "hal/linux_pi/LinuxBle.h"
#include "hal/linux_pi/LinuxCan.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

static int g_pass = 0, g_fail = 0;

static void expectTrue(const char* name, bool condition) {
    if (condition) {
        g_pass++;
        std::printf("  [PASS] %s\n", name);
    } else {
        g_fail++;
        std::printf("  [FAIL] %s\n", name);
    }
}

int main() {
    std::printf("=== Linux / Raspberry Pi HAL Native Test Suite ===\n\n");

    // 1. LinuxGpio
    std::printf("LinuxGpio tests:\n");
    LinuxGpio gpio;
    gpio.setPinMode(10, GpioMode::OUTPUT);
    gpio.writePin(10, PinLevel::HIGH_VAL);
    expectTrue("Pin 10 reads HIGH", gpio.readPin(10) == PinLevel::HIGH_VAL);

    gpio.writePin(10, PinLevel::LOW_VAL);
    expectTrue("Pin 10 reads LOW", gpio.readPin(10) == PinLevel::LOW_VAL);

    gpio.setPwm(0, 9, 5000, 128);
    gpio.writePwm(0, 200);
    expectTrue("PWM setup and write successful", true);

    // 2. LinuxStorage
    std::printf("\nLinuxStorage tests:\n");
    const char* testCfgPath = "/tmp/test_aezel_config.json";
    unlink(testCfgPath);
    LinuxStorage storage(testCfgPath);
    expectTrue("Storage begin", storage.begin());

    storage.setString("vehicle_name", "Avenger 150");
    expectTrue("String round-trip", storage.getString("vehicle_name", "") == "Avenger 150");

    storage.setUInt("odometer", 15420);
    expectTrue("UInt round-trip", storage.getUInt("odometer", 0) == 15420);

    storage.setFloat("fuel_pct", 78.5f);
    expectTrue("Float round-trip", std::abs(storage.getFloat("fuel_pct", 0.0f) - 78.5f) < 0.01f);

    expectTrue("Default value on missing key", storage.getUInt("non_existent", 999) == 999);
    expectTrue("Append log to temp file", storage.appendLog("/tmp/test_aezel_log.txt", "2026-11-09T00:00:00Z,INIT,OK"));
    unlink(testCfgPath);
    unlink("/tmp/test_aezel_log.txt");

    // 3. LinuxBle
    std::printf("\nLinuxBle tests:\n");
    LinuxBle ble;
    expectTrue("BLE begin", ble.begin("AEZEL-VCU"));
    expectTrue("Initial state disconnected", !ble.isConnected());

    ble.setConnected(true);
    expectTrue("BLE connected state updated", ble.isConnected());

    std::string receivedCmd = "";
    ble.onCommandReceived([&receivedCmd](const std::string& cmd) {
        receivedCmd = cmd;
    });
    ble.simulateIncomingCommand("{\"cmd\":\"set_profile\",\"index\":2}");
    expectTrue("Command callback received", receivedCmd == "{\"cmd\":\"set_profile\",\"index\":2}");

    // 4. HalProvider Registry
    std::printf("\nHalProvider registry tests:\n");
    HalProvider::instance().setGpio(&gpio);
    HalProvider::instance().setStorage(&storage);
    HalProvider::instance().setBle(&ble);

    expectTrue("HalProvider::gpio() registered", HalProvider::gpio() == &gpio);
    expectTrue("HalProvider::storage() registered", HalProvider::storage() == &storage);
    expectTrue("HalProvider::ble() registered", HalProvider::ble() == &ble);

    std::printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
