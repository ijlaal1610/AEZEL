// ============================================================================
//  test_vehicle_profile.cpp — native unit tests for Multi-Vehicle Profiles
// ============================================================================
#include "VehicleProfile.h"
#include "VehiclePresets.h"
#include "VehicleConfig.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

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

static void expectEqualStr(const char* name, const char* actual, const char* expected) {
    bool ok = (std::strcmp(actual, expected) == 0);
    if (ok) {
        g_pass++;
        std::printf("  [PASS] %s (\"%s\")\n", name, actual);
    } else {
        g_fail++;
        std::printf("  [FAIL] %s (got \"%s\", expected \"%s\")\n", name, actual, expected);
    }
}

int main() {
    std::printf("=== Multi-Vehicle Profile & Preset Native Test Suite ===\n\n");

    // 1. Preset 0: Bike Direct / Non-OBD
    std::printf("Preset 0 - Bike (Direct/Non-OBD):\n");
    expectTrue("Vehicle type is BIKE", PRESET_BIKE_DIRECT.vehicleType == VehicleType::BIKE);
    expectTrue("Data source is DIRECT_SENSOR", PRESET_BIKE_DIRECT.dataSource == DataSourceMode::DIRECT_SENSOR);
    expectTrue("Transmission is MANUAL", PRESET_BIKE_DIRECT.transmission == TransmissionType::MANUAL);
    expectTrue("Tacho max RPM is 12000", PRESET_BIKE_DIRECT.tachoMaxRpm == 12000);
    expectTrue("Speedo max is 180 km/h", PRESET_BIKE_DIRECT.speedoMaxKmh == 180);
    expectTrue("Max gear is 5", PRESET_BIKE_DIRECT.maxGear == 5);
    expectTrue("Has lean angle enabled", PRESET_BIKE_DIRECT.hasLeanAngle == true);
    expectTrue("Has side stand enabled", PRESET_BIKE_DIRECT.hasSideStand == true);
    expectTrue("Has clutch enabled", PRESET_BIKE_DIRECT.hasClutch == true);
    expectTrue("Throttle bar disabled", PRESET_BIKE_DIRECT.hasThrottleBar == false);

    // 2. Preset 1: Bike OBD Hybrid
    std::printf("\nPreset 1 - Bike (OBD Hybrid):\n");
    expectTrue("Vehicle type is BIKE", PRESET_BIKE_OBD.vehicleType == VehicleType::BIKE);
    expectTrue("Data source is HYBRID", PRESET_BIKE_OBD.dataSource == DataSourceMode::HYBRID);
    expectTrue("Tacho max RPM is 14000", PRESET_BIKE_OBD.tachoMaxRpm == 14000);
    expectTrue("Max gear is 6", PRESET_BIKE_OBD.maxGear == 6);
    expectTrue("Throttle bar enabled via OBD", PRESET_BIKE_OBD.hasThrottleBar == true);
    expectTrue("Engine load bar enabled via OBD", PRESET_BIKE_OBD.hasEngineLoadBar == true);

    // 3. Preset 2: Scooter Direct CVT
    std::printf("\nPreset 2 - Scooter (Direct/CVT):\n");
    expectTrue("Vehicle type is SCOOTER", PRESET_SCOOTER_DIRECT.vehicleType == VehicleType::SCOOTER);
    expectTrue("Transmission is CVT", PRESET_SCOOTER_DIRECT.transmission == TransmissionType::CVT);
    expectTrue("Max gear is 0 (CVT)", PRESET_SCOOTER_DIRECT.maxGear == 0);
    expectTrue("Clutch lever disabled for CVT", PRESET_SCOOTER_DIRECT.hasClutch == false);
    expectTrue("Gear indicator disabled for CVT", PRESET_SCOOTER_DIRECT.hasGearIndicator == false);
    expectTrue("Has tilt alarm enabled", PRESET_SCOOTER_DIRECT.hasTiltAlarm == true);

    // 4. Preset 4: Car Manual OBD
    std::printf("\nPreset 4 - Car Manual (OBD-II):\n");
    expectTrue("Vehicle type is CAR", PRESET_CAR_OBD.vehicleType == VehicleType::CAR);
    expectTrue("Transmission is MANUAL", PRESET_CAR_OBD.transmission == TransmissionType::MANUAL);
    expectTrue("Tacho max RPM is 7000", PRESET_CAR_OBD.tachoMaxRpm == 7000);
    expectTrue("Speedo max is 220 km/h", PRESET_CAR_OBD.speedoMaxKmh == 220);
    expectTrue("Lean angle disabled for Car", PRESET_CAR_OBD.hasLeanAngle == false);
    expectTrue("Side stand disabled for Car", PRESET_CAR_OBD.hasSideStand == false);
    expectTrue("Tilt alarm disabled (uses G-force)", PRESET_CAR_OBD.hasTiltAlarm == false);

    // 5. Preset 5: Car Automatic OBD
    std::printf("\nPreset 5 - Car Auto (OBD-II P-R-N-D):\n");
    expectTrue("Vehicle type is CAR", PRESET_CAR_AUTO_OBD.vehicleType == VehicleType::CAR);
    expectTrue("Transmission is AUTOMATIC", PRESET_CAR_AUTO_OBD.transmission == TransmissionType::AUTOMATIC);
    expectTrue("Numeric gear indicator disabled", PRESET_CAR_AUTO_OBD.hasGearIndicator == false);
    expectTrue("Speedo max is 240 km/h", PRESET_CAR_AUTO_OBD.speedoMaxKmh == 240);

    // 6. VehicleConfig Runtime Switching
    std::printf("\nVehicleConfig runtime switching:\n");
    auto& vcfg = VehicleConfig::instance();
    vcfg.setProfileByIndex(0);
    expectTrue("Profile 0 is bike", vcfg.isBike());
    expectTrue("Profile 0 uses direct sensors", vcfg.usesDirectSensors());
    expectTrue("Profile 0 has clutch", vcfg.hasClutch());

    vcfg.setProfileByIndex(2);
    expectTrue("Profile 2 is scooter", vcfg.isScooter());
    expectTrue("Profile 2 transmission is CVT", vcfg.transmission() == TransmissionType::CVT);
    expectTrue("Profile 2 has no gear indicator", !vcfg.hasGearIndicator());

    vcfg.setProfileByIndex(4);
    expectTrue("Profile 4 is car", vcfg.isCar());
    expectTrue("Profile 4 uses OBD", vcfg.usesObd());
    expectTrue("Profile 4 has no lean angle", !vcfg.hasLeanAngle());

    // 7. String helpers
    std::printf("\nString conversion helpers:\n");
    expectEqualStr("vehicleTypeToString(BIKE)", vehicleTypeToString(VehicleType::BIKE), "Bike");
    expectEqualStr("vehicleTypeToString(SCOOTER)", vehicleTypeToString(VehicleType::SCOOTER), "Scooter");
    expectEqualStr("vehicleTypeToString(CAR)", vehicleTypeToString(VehicleType::CAR), "Car");
    expectEqualStr("transmissionTypeToString(CVT)", transmissionTypeToString(TransmissionType::CVT), "CVT (Automatic)");
    expectEqualStr("dataSourceModeToString(HYBRID)", dataSourceModeToString(DataSourceMode::HYBRID), "Hybrid (OBD + Sensors)");

    std::printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
