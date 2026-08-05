#pragma once
// ============================================================================
//  DisplayManager — owns the LVGL UI: screen composition, widgets, theme
//  switching, and the 60fps render loop. Runs pinned to CORE_REALTIME so
//  rendering never stalls behind BLE/GPS/storage I/O on the other core.
//
//  Screen structure (see docs/screen_flow.md for the full state diagram):
//    MainDashboard <-> TripInfo <-> Notifications <-> Settings  (cycle)
//  Navigation isn't built yet (Phase 3, needs a routing-engine decision
//  first) so it's skipped in the current cycle order. Swipe left/right,
//  the physical MODE button, and the rotary encoder all drive the same
//  next/prev navigation; MODE long-press jumps straight to Settings from
//  anywhere; a CRITICAL notification force-switches to Notifications and
//  blocks navigation away until acknowledged.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"
#include <lvgl.h>

enum class Screen : uint8_t { MAIN_DASHBOARD, TRIP_INFO, NOTIFICATIONS, SETTINGS };

class DisplayManager {
public:
    static DisplayManager& instance() { static DisplayManager d; return d; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    void goToScreen(Screen s);
    void showGoodbyeScreen();   // called synchronously by PowerManager before sleep

private:
    DisplayManager() = default;

    void buildMainDashboard();
    void buildTripInfoScreen();
    void buildNotificationsScreen();
    void buildSettingsScreen();
    void applyTheme(ThemeMode mode);
    void refreshWidgetsFromState();

    // Navigation
    void nextScreen();
    void prevScreen();
    void handlePhysicalInputs();
    static void gestureEventCb(lv_event_t* e);
    static lv_obj_t* attachGestureNav(lv_obj_t* screen);   // helper: creates screen + wires swipe

    // Settings actions (bound to button click callbacks)
    static void onThemeButtonClicked(lv_event_t* e);
    static void onRideModeButtonClicked(lv_event_t* e);
    static void onResetTripAClicked(lv_event_t* e);
    static void onResetTripBClicked(lv_event_t* e);
    static void onNotificationRowClicked(lv_event_t* e);

    void refreshNotificationsList();
    void refreshSettingsLabels();

    // --- Main Dashboard widgets ---
    lv_obj_t* _screenMain = nullptr;
    lv_obj_t* _labelSpeed = nullptr;
    lv_obj_t* _labelSpeedUnit = nullptr;
    lv_obj_t* _arcRpm = nullptr;
    lv_obj_t* _labelGear = nullptr;
    lv_obj_t* _labelClock = nullptr;
    lv_obj_t* _labelTrip = nullptr;
    lv_obj_t* _barFuel = nullptr;
    lv_obj_t* _labelEngineTemp = nullptr;
    lv_obj_t* _iconLeftIndicator = nullptr;
    lv_obj_t* _iconRightIndicator = nullptr;
    lv_obj_t* _iconNeutral = nullptr;
    lv_obj_t* _iconHighBeam = nullptr;
    lv_obj_t* _labelWarningBanner = nullptr;

    // --- Trip Info screen widgets ---
    lv_obj_t* _screenTripInfo = nullptr;
    lv_obj_t* _labelTripA = nullptr;
    lv_obj_t* _labelTripB = nullptr;
    lv_obj_t* _labelOdometer = nullptr;
    lv_obj_t* _labelRideTimer = nullptr;
    lv_obj_t* _labelAvgSpeed = nullptr;
    lv_obj_t* _labelMaxSpeed = nullptr;
    lv_obj_t* _labelFuelRange = nullptr;
    lv_obj_t* _labelFuelConsumption = nullptr;

    // --- Notifications screen widgets ---
    lv_obj_t* _screenNotifications = nullptr;
    lv_obj_t* _notifList = nullptr;         // lv_list — one row per queued notification
    lv_obj_t* _labelNoNotifications = nullptr;

    // --- Settings screen widgets ---
    lv_obj_t* _screenSettings = nullptr;
    lv_obj_t* _labelThemeValue = nullptr;
    lv_obj_t* _labelRideModeValue = nullptr;
    lv_obj_t* _labelSdStatus = nullptr;
    lv_obj_t* _labelGpsStatus = nullptr;
    lv_obj_t* _labelBleStatus = nullptr;
    lv_obj_t* _sliderBrightness = nullptr;

    Screen _currentScreen = Screen::MAIN_DASHBOARD;
    uint32_t _lastRenderMs = 0;

    // Physical button/rotary debounce state
    bool _lastModeBtn = false, _lastOkBtn = false;
    uint32_t _modeBtnDownMs = 0;
    bool _modeLongPressFired = false;
    static constexpr uint32_t LONG_PRESS_MS = 1000;

    // Settings state (persisted via StorageManager in a fuller build — kept
    // as in-memory members here since the NVS keys for these are reserved
    // but not yet wired, see docs/nvs_layout.md)
    ThemeMode _selectedTheme = ThemeMode::MODERN_DIGITAL;
    RideMode _selectedRideMode = RideMode::CITY;
};

