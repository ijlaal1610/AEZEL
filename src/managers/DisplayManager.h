#pragma once
// ============================================================================
//  DisplayManager — owns the LVGL UI: screen composition, widgets, theme
//  switching, and the 60fps render loop. Runs pinned to CORE_REALTIME so
//  rendering never stalls behind BLE/GPS/storage I/O on the other core.
//
//  Screen structure (see docs/screen_flow.md for the full state diagram):
//    MainDashboard -> TripInfo -> Navigation -> Notifications -> Settings
//  Swipe left/right cycles screens; rotary encoder / physical buttons mirror
//  the same navigation for gloved-hand use.
// ============================================================================
#include <Arduino.h>
#include "DataModel.h"
#include <lvgl.h>

enum class Screen : uint8_t { LOCKSCREEN, MAIN_DASHBOARD, TRIP_INFO, NAVIGATION, NOTIFICATIONS, SETTINGS };

class DisplayManager {
public:
    static DisplayManager& instance() { static DisplayManager d; return d; }

    void begin();
    static void taskEntry(void* pv);
    void tick();

    void goToScreen(Screen s);
    void showGoodbyeScreen();   // called synchronously by PowerManager before sleep

    void toggleMenuDrawer();

private:
    DisplayManager() = default;

    void buildLockScreen();
    void buildMainDashboard();
    void buildTripInfoScreen();
    void buildNavigationScreen();
    void buildNotificationsScreen();
    void buildSettingsScreen();
    void buildMenuDrawer();
    void applyTheme(ThemeMode mode);
    void refreshWidgetsFromState();

    // LVGL objects for main dashboard
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

    // LVGL objects for Trip Info Screen
    lv_obj_t* _screenTrip = nullptr;
    lv_obj_t* _labelTripMaxSpeed = nullptr;
    lv_obj_t* _labelTripAvgSpeed = nullptr;
    lv_obj_t* _labelTripRideTime = nullptr;
    lv_obj_t* _labelTripFuelRange = nullptr;

    // LVGL objects for Turn-by-Turn Navigation Screen
    lv_obj_t* _screenNav = nullptr;
    lv_obj_t* _labelNavTurnIcon = nullptr;
    lv_obj_t* _labelNavDistance = nullptr;
    lv_obj_t* _labelNavStreet = nullptr;
    lv_obj_t* _labelNavEta = nullptr;

    // LVGL objects for Notifications Screen
    lv_obj_t* _screenNotif = nullptr;
    lv_obj_t* _labelNotifTitle = nullptr;
    lv_obj_t* _labelNotifBody = nullptr;

    // LVGL objects for Settings & Calibration Screen
    lv_obj_t* _screenSettings = nullptr;
    lv_obj_t* _switchSpeedo = nullptr;
    lv_obj_t* _switchFocus = nullptr;
    lv_obj_t* _btnOtaWifi = nullptr;

    // LVGL objects for Security Lockscreen
    lv_obj_t* _screenLock = nullptr;
    lv_obj_t* _labelLockPin = nullptr;
    lv_obj_t* _labelLockStatus = nullptr;

    // LVGL objects for Slide-Out Navigation Drawer Overlay
    lv_obj_t* _drawerContainer = nullptr;
    bool      _isDrawerOpen = false;

    Screen _currentScreen = Screen::MAIN_DASHBOARD;
    uint32_t _lastRenderMs = 0;
};
