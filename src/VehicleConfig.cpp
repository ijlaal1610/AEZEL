#include "VehicleConfig.h"
#if defined(ESP32) || defined(ARDUINO)
#include <Preferences.h>
static Preferences s_prefs;
#endif
#include <cstring>

VehicleConfig::VehicleConfig() {
    // Default fallback to non-OBD bike (Bajaj Avenger 150)
    _activeProfile = PRESET_BIKE_DIRECT;
}

void VehicleConfig::begin() {
    loadFromNvs();
}

bool VehicleConfig::setProfileByIndex(size_t index) {
    if (index >= TOTAL_PRESETS) return false;
    _activeProfile = VEHICLE_PRESET_CATALOG[index];
    saveToNvs();
    return true;
}

bool VehicleConfig::setProfile(const VehicleProfile& profile) {
    _activeProfile = profile;
    saveToNvs();
    return true;
}

void VehicleConfig::loadFromNvs() {
#if defined(ESP32) || defined(ARDUINO)
    if (s_prefs.begin("aezel_veh", true)) {
        size_t presetIdx = s_prefs.getUInt("preset_idx", 0);
        if (presetIdx < TOTAL_PRESETS) {
            _activeProfile = VEHICLE_PRESET_CATALOG[presetIdx];
        }

        // Allow fine-grained custom overrides from NVS if saved
        _activeProfile.vehicleType = (VehicleType)s_prefs.getUChar("v_type", (uint8_t)_activeProfile.vehicleType);
        _activeProfile.dataSource = (DataSourceMode)s_prefs.getUChar("d_src", (uint8_t)_activeProfile.dataSource);
        _activeProfile.transmission = (TransmissionType)s_prefs.getUChar("trans", (uint8_t)_activeProfile.transmission);
        _activeProfile.wheelCircumferenceM = s_prefs.getFloat("wheel_m", _activeProfile.wheelCircumferenceM);
        _activeProfile.fuelTankCapacityL = s_prefs.getFloat("tank_l", _activeProfile.fuelTankCapacityL);
        _activeProfile.tachoMaxRpm = s_prefs.getUShort("tacho_max", _activeProfile.tachoMaxRpm);
        _activeProfile.redlineRpm = s_prefs.getUShort("redline", _activeProfile.redlineRpm);
        _activeProfile.speedoMaxKmh = s_prefs.getUShort("speedo_max", _activeProfile.speedoMaxKmh);

        s_prefs.end();
    }
#endif
}

void VehicleConfig::saveToNvs() {
#if defined(ESP32) || defined(ARDUINO)
    if (s_prefs.begin("aezel_veh", false)) {
        // Find if matches a known catalog preset
        size_t matchedIdx = 0;
        for (size_t i = 0; i < TOTAL_PRESETS; ++i) {
            if (_activeProfile.vehicleType == VEHICLE_PRESET_CATALOG[i].vehicleType &&
                _activeProfile.dataSource == VEHICLE_PRESET_CATALOG[i].dataSource &&
                _activeProfile.transmission == VEHICLE_PRESET_CATALOG[i].transmission) {
                matchedIdx = i;
                break;
            }
        }
        s_prefs.putUInt("preset_idx", matchedIdx);
        s_prefs.putUChar("v_type", (uint8_t)_activeProfile.vehicleType);
        s_prefs.putUChar("d_src", (uint8_t)_activeProfile.dataSource);
        s_prefs.putUChar("trans", (uint8_t)_activeProfile.transmission);
        s_prefs.putFloat("wheel_m", _activeProfile.wheelCircumferenceM);
        s_prefs.putFloat("tank_l", _activeProfile.fuelTankCapacityL);
        s_prefs.putUShort("tacho_max", _activeProfile.tachoMaxRpm);
        s_prefs.putUShort("redline", _activeProfile.redlineRpm);
        s_prefs.putUShort("speedo_max", _activeProfile.speedoMaxKmh);
        s_prefs.end();
    }
#endif
}
