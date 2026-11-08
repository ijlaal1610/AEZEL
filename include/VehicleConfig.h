#pragma once
// ============================================================================
//  VehicleConfig.h — Active Vehicle Profile Manager & Persistence
// ============================================================================
#include "VehicleProfile.h"
#include "VehiclePresets.h"

class VehicleConfig {
public:
    static VehicleConfig& instance() {
        static VehicleConfig s_instance;
        return s_instance;
    }

    void begin();
    const VehicleProfile& active() const { return _activeProfile; }

    bool setProfileByIndex(size_t index);
    bool setProfile(const VehicleProfile& profile);

    // Helpers
    VehicleType type() const { return _activeProfile.vehicleType; }
    DataSourceMode dataSource() const { return _activeProfile.dataSource; }
    TransmissionType transmission() const { return _activeProfile.transmission; }

    bool isBike() const { return _activeProfile.vehicleType == VehicleType::BIKE; }
    bool isScooter() const { return _activeProfile.vehicleType == VehicleType::SCOOTER; }
    bool isCar() const { return _activeProfile.vehicleType == VehicleType::CAR; }

    bool usesDirectSensors() const {
        return _activeProfile.dataSource == DataSourceMode::DIRECT_SENSOR ||
               _activeProfile.dataSource == DataSourceMode::HYBRID;
    }

    bool usesObd() const {
        return _activeProfile.dataSource == DataSourceMode::OBD_ONLY ||
               _activeProfile.dataSource == DataSourceMode::HYBRID;
    }

    bool hasLeanAngle() const { return _activeProfile.hasLeanAngle; }
    bool hasSideStand() const { return _activeProfile.hasSideStand; }
    bool hasClutch() const { return _activeProfile.hasClutch; }
    bool hasGearIndicator() const { return _activeProfile.hasGearIndicator; }
    bool hasThrottleBar() const { return _activeProfile.hasThrottleBar; }
    bool hasEngineLoadBar() const { return _activeProfile.hasEngineLoadBar; }

private:
    VehicleConfig();
    void loadFromNvs();
    void saveToNvs();

    VehicleProfile _activeProfile;
};
