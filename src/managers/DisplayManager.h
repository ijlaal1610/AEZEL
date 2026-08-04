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

enum class Screen : uint8_t { MAIN_DASHBOARD, TRIP_INFO, NAVIGATION, NOTIFICATIONS, SETTINGS };

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
    void buildNavigationScreen();
    void buildNotificationBanner();
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

    // LVGL objects for Turn-by-Turn Navigation Screen
    lv_obj_t* _screenNav = nullptr;
    lv_obj_t* _labelNavTurnIcon = nullptr;
    lv_obj_t* _labelNavDistance = nullptr;
    lv_obj_t* _labelNavStreet = nullptr;
    lv_obj_t* _labelNavEta = nullptr;

    Screen _currentScreen = Screen::MAIN_DASHBOARD;
    uint32_t _lastRenderMs = 0;
};
