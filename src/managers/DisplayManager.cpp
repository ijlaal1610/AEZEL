#include "DisplayManager.h"
#include "Config.h"
#include "NotificationManager.h"
#include "StorageManager.h"
#include "OtaManager.h"
#include "VehicleConfig.h"
#include <TFT_eSPI.h>

static TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t drawBuf;
static lv_color_t buf1[SCREEN_W * 40];   // partial buffer (40 lines)
static lv_disp_drv_t dispDriver;

// ----------------------------------------------------------------- LVGL glue
static void lvglFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* colorP) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)&colorP->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

static void lvglTouchReadCb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    uint16_t tx, ty;
    bool touched = tft.getTouch(&tx, &ty);
    data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    if (touched) { data->point.x = tx; data->point.y = ty; }
}

// ---------------------------------------------------------------- Settings Callbacks
static void speedoSwitchCb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    SharedState::instance().update([state](VehicleState& s) { s.showSpeedometer = state; });
    StorageManager::instance().saveUserSettings();
}

static void focusSwitchCb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    SharedState::instance().update([state](VehicleState& s) { s.focusMode = state; });
    StorageManager::instance().saveUserSettings();
}

static void notifOverlaySwitchCb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    SharedState::instance().update([state](VehicleState& s) { s.allowNotifOverlay = state; });
    StorageManager::instance().saveUserSettings();
}

static void lockscreenSwitchCb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    SharedState::instance().update([state](VehicleState& s) { s.enableLockscreen = state; });
    StorageManager::instance().saveUserSettings();
}

static void otaBtnCb(lv_event_t* e) {
    OtaManager::instance().enableWifiAp();
    NotificationManager::instance().push("OTA AP ACTIVE", "SSID: AEZEL-VCU-AP (192.168.4.1)", NotifPriority::INFO);
}

static void profileCycleBtnCb(lv_event_t* e) {
    static size_t s_presetIdx = 0;
    s_presetIdx = (s_presetIdx + 1) % TOTAL_PRESETS;
    VehicleConfig::instance().setProfileByIndex(s_presetIdx);
    DisplayManager::instance().updateProfileWidgets();
    const auto& prof = VehicleConfig::instance().active();
    NotificationManager::instance().push("PROFILE SWITCHED", prof.name, NotifPriority::INFO);
}


// ---------------------------------------------------------------------------
void DisplayManager::begin() {
    _lvglMutex = xSemaphoreCreateRecursiveMutex();

    tft.init();
    tft.setRotation(1);   // landscape
    ledcSetup(LEDC_CHANNEL_BACKLIGHT, 5000, 8);
    ledcAttachPin(PIN_TFT_BL, LEDC_CHANNEL_BACKLIGHT);
    ledcWrite(LEDC_CHANNEL_BACKLIGHT, 200);    // initial backlight ~78%

    lv_init();
    lv_disp_draw_buf_init(&drawBuf, buf1, nullptr, SCREEN_W * 40);

    lv_disp_drv_init(&dispDriver);
    dispDriver.hor_res = SCREEN_W;
    dispDriver.ver_res = SCREEN_H;
    dispDriver.flush_cb = lvglFlushCb;
    dispDriver.draw_buf = &drawBuf;
    lv_disp_drv_register(&dispDriver);

    static lv_indev_drv_t indevDriver;
    lv_indev_drv_init(&indevDriver);
    indevDriver.type = LV_INDEV_TYPE_POINTER;
    indevDriver.read_cb = lvglTouchReadCb;
    lv_indev_drv_register(&indevDriver);

    if (lvglLock(100)) {
        buildLockScreen();
        buildMainDashboard();
        buildTripInfoScreen();
        buildNavigationScreen();
        buildNotificationsScreen();
        buildSettingsScreen();
        buildMenuDrawer();
        applyTheme(ThemeMode::MODERN_DIGITAL);
        goToScreen(Screen::MAIN_DASHBOARD);
        playStartupSweepAnimation();
        lvglUnlock();
    }
}

void DisplayManager::playStartupSweepAnimation() {
    if (!_arcRpm) return;
    const uint16_t maxRpm = VehicleConfig::instance().active().tachoMaxRpm;

    // Tachometer Arc Sweep: 0 -> Max -> 0
    for (int r = 0; r <= maxRpm; r += (maxRpm / 30)) {
        lv_arc_set_value(_arcRpm, r);
        lv_timer_handler();
        delay(15);
    }
    for (int r = maxRpm; r >= 0; r -= (maxRpm / 30)) {
        lv_arc_set_value(_arcRpm, r);
        lv_timer_handler();
        delay(15);
    }
    lv_arc_set_value(_arcRpm, 0);
    lv_timer_handler();
    delay(100);
}

void DisplayManager::taskEntry(void* pv) {
    DisplayManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(16);   // ~60 Hz
    for (;;) {
        self.tick();
        vTaskDelay(period);
    }
}

void DisplayManager::tick() {
    if (lvglLock(20)) {
        refreshWidgetsFromState();
        lv_timer_handler();
        lvglUnlock();
    }
}

// ----------------------------------------------------------- UI Composition
static void drawerBtnCb(lv_event_t* e) {
    Screen target = (Screen)(uintptr_t)lv_event_get_user_data(e);
    DisplayManager::instance().goToScreen(target);
    DisplayManager::instance().toggleMenuDrawer();
}

static void screenGestureCb(lv_event_t* e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    // Swipe RIGHT opens the left drawer; Swipe LEFT closes it
    if (dir == LV_DIR_RIGHT || dir == LV_DIR_LEFT) {
        DisplayManager::instance().toggleMenuDrawer();
    }
}

void DisplayManager::buildMenuDrawer() {
    _drawerContainer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_drawerContainer, 180, SCREEN_H);
    lv_obj_align(_drawerContainer, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(_drawerContainer, lv_color_hex(0x0F141C), 0);
    lv_obj_set_style_border_color(_drawerContainer, lv_color_hex(0x00D4FF), 0);
    lv_obj_set_style_border_width(_drawerContainer, 2, 0);
    lv_obj_set_style_pad_all(_drawerContainer, 8, 0);
    lv_obj_add_flag(_drawerContainer, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* title = lv_label_create(_drawerContainer);
    lv_label_set_text(title, "QUICK MENU");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00D4FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    const char* labels[] = { "1. Dashboard", "2. Trip Analytics", "3. Turn Navigation", "4. Notifications", "5. Settings", "6. Lock Cockpit" };
    Screen screens[] = { Screen::MAIN_DASHBOARD, Screen::TRIP_INFO, Screen::NAVIGATION, Screen::NOTIFICATIONS, Screen::SETTINGS, Screen::LOCKSCREEN };

    for (int i = 0; i < 6; i++) {
        lv_obj_t* btn = lv_btn_create(_drawerContainer);
        lv_obj_set_size(btn, 160, 36);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 42 + i * 44);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1B2432), 0);
        lv_obj_add_event_cb(btn, drawerBtnCb, LV_EVENT_CLICKED, (void*)(uintptr_t)screens[i]);

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_center(lbl);
    }
}

void DisplayManager::toggleMenuDrawer() {
    if (!_drawerContainer) return;
    if (lvglLock(50)) {
        _isDrawerOpen = !_isDrawerOpen;
        if (_isDrawerOpen) {
            lv_obj_clear_flag(_drawerContainer, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(_drawerContainer);
        } else {
            lv_obj_add_flag(_drawerContainer, LV_OBJ_FLAG_HIDDEN);
        }
        lvglUnlock();
    }
}

void DisplayManager::buildLockScreen() {
    _screenLock = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenLock, lv_color_hex(0x05070A), 0);
    lv_obj_add_event_cb(_screenLock, screenGestureCb, LV_EVENT_GESTURE, nullptr);

    lv_obj_t* icon = lv_label_create(_screenLock);
    lv_label_set_text(icon, LV_SYMBOL_SETTINGS " " LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xFF1744), 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -60);

    lv_obj_t* title = lv_label_create(_screenLock);
    lv_label_set_text(title, "AEZEL COCKPIT LOCKED");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF1744), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -10);

    _labelLockStatus = lv_label_create(_screenLock);
    lv_label_set_text(_labelLockStatus, "Connect paired phone or enter PIN to unlock");
    lv_obj_set_style_text_color(_labelLockStatus, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelLockStatus, LV_ALIGN_CENTER, 0, 25);
}

void DisplayManager::buildMainDashboard() {
    _screenMain = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenMain, lv_color_black(), 0);
    lv_obj_add_event_cb(_screenMain, screenGestureCb, LV_EVENT_GESTURE, nullptr);

    const auto& profile = VehicleConfig::instance().active();

    // Arc gauge for RPM: dynamic max scale per vehicle profile
    _arcRpm = lv_arc_create(_screenMain);
    lv_obj_set_size(_arcRpm, 260, 260);
    lv_arc_set_rotation(_arcRpm, 135);
    lv_arc_set_bg_angles(_arcRpm, 0, 270);
    lv_arc_set_range(_arcRpm, 0, profile.tachoMaxRpm);
    lv_obj_remove_style(_arcRpm, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(_arcRpm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(_arcRpm, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arcRpm, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(_arcRpm, lv_color_hex(0x1B2432), LV_PART_MAIN);
    lv_obj_align(_arcRpm, LV_ALIGN_CENTER, 0, 10);

    // Center speed readout
    _labelSpeed = lv_label_create(_screenMain);
    lv_label_set_text(_labelSpeed, "0");
    lv_obj_set_style_text_font(_labelSpeed, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(_labelSpeed, lv_color_white(), 0);
    lv_obj_align(_labelSpeed, LV_ALIGN_CENTER, 0, -10);

    _labelSpeedUnit = lv_label_create(_screenMain);
    lv_label_set_text(_labelSpeedUnit, "km/h");
    lv_obj_set_style_text_color(_labelSpeedUnit, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelSpeedUnit, LV_ALIGN_CENTER, 0, 30);

    // Gear indicator / transmission state
    _labelGear = lv_label_create(_screenMain);
    lv_label_set_text(_labelGear, "N");
    lv_obj_set_style_text_font(_labelGear, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_labelGear, lv_color_hex(0x00E676), 0);
    lv_obj_align(_labelGear, LV_ALIGN_CENTER, 0, 70);

    // Top status row: indicators, clock
    _labelClock = lv_label_create(_screenMain);
    lv_label_set_text(_labelClock, "00:00");
    lv_obj_set_style_text_color(_labelClock, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelClock, LV_ALIGN_TOP_MID, 0, 10);

    _iconLeftIndicator = lv_label_create(_screenMain);
    lv_label_set_text(_iconLeftIndicator, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(_iconLeftIndicator, lv_color_hex(0x00E676), 0);
    lv_obj_align(_iconLeftIndicator, LV_ALIGN_TOP_LEFT, 20, 10);
    lv_obj_add_flag(_iconLeftIndicator, LV_OBJ_FLAG_HIDDEN);

    _iconRightIndicator = lv_label_create(_screenMain);
    lv_label_set_text(_iconRightIndicator, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(_iconRightIndicator, lv_color_hex(0x00E676), 0);
    lv_obj_align(_iconRightIndicator, LV_ALIGN_TOP_RIGHT, -20, 10);
    lv_obj_add_flag(_iconRightIndicator, LV_OBJ_FLAG_HIDDEN);

    _iconNeutral = lv_label_create(_screenMain);
    lv_label_set_text(_iconNeutral, "N");
    lv_obj_set_style_text_color(_iconNeutral, lv_color_hex(0x00E676), 0);
    lv_obj_align(_iconNeutral, LV_ALIGN_TOP_LEFT, 60, 10);

    _iconHighBeam = lv_label_create(_screenMain);
    lv_label_set_text(_iconHighBeam, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_color(_iconHighBeam, lv_color_hex(0x2979FF), 0);
    lv_obj_align(_iconHighBeam, LV_ALIGN_TOP_RIGHT, -60, 10);
    lv_obj_add_flag(_iconHighBeam, LV_OBJ_FLAG_HIDDEN);

    // Bottom telemetry bar: trip / odometer
    _labelTrip = lv_label_create(_screenMain);
    lv_label_set_text(_labelTrip, "A 0.0 km  |  ODO 0 km");
    lv_obj_set_style_text_color(_labelTrip, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelTrip, LV_ALIGN_BOTTOM_MID, 0, -10);

    // Vertical Fuel gauge
    _barFuel = lv_bar_create(_screenMain);
    lv_obj_set_size(_barFuel, 12, 100);
    lv_obj_align(_barFuel, LV_ALIGN_LEFT_MID, 20, 10);
    lv_bar_set_range(_barFuel, 0, 100);
    lv_obj_set_style_bg_color(_barFuel, lv_color_hex(0x1B2432), LV_PART_MAIN);
    lv_obj_set_style_bg_color(_barFuel, lv_color_hex(0x00E676), LV_PART_INDICATOR);

    // Engine temperature readout
    _labelEngineTemp = lv_label_create(_screenMain);
    lv_label_set_text(_labelEngineTemp, "--C");
    lv_obj_set_style_text_color(_labelEngineTemp, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelEngineTemp, LV_ALIGN_RIGHT_MID, -20, 10);

    // OBD-II Powertrain: Throttle & Engine Load bars
    _barThrottle = lv_bar_create(_screenMain);
    lv_obj_set_size(_barThrottle, 8, 80);
    lv_obj_align(_barThrottle, LV_ALIGN_RIGHT_MID, -45, 10);
    lv_bar_set_range(_barThrottle, 0, 100);
    lv_obj_set_style_bg_color(_barThrottle, lv_color_hex(0x1B2432), LV_PART_MAIN);
    lv_obj_set_style_bg_color(_barThrottle, lv_color_hex(0x00D4FF), LV_PART_INDICATOR);
    if (!profile.hasThrottleBar) lv_obj_add_flag(_barThrottle, LV_OBJ_FLAG_HIDDEN);

    _barEngineLoad = lv_bar_create(_screenMain);
    lv_obj_set_size(_barEngineLoad, 8, 80);
    lv_obj_align(_barEngineLoad, LV_ALIGN_RIGHT_MID, -58, 10);
    lv_bar_set_range(_barEngineLoad, 0, 100);
    lv_obj_set_style_bg_color(_barEngineLoad, lv_color_hex(0x1B2432), LV_PART_MAIN);
    lv_obj_set_style_bg_color(_barEngineLoad, lv_color_hex(0xFF8A00), LV_PART_INDICATOR);
    if (!profile.hasEngineLoadBar) lv_obj_add_flag(_barEngineLoad, LV_OBJ_FLAG_HIDDEN);

    // Warning Banner Overlay
    _labelWarningBanner = lv_label_create(_screenMain);
    lv_label_set_text(_labelWarningBanner, "");
    lv_obj_set_style_bg_color(_labelWarningBanner, lv_color_hex(0xD50000), 0);
    lv_obj_set_style_bg_opa(_labelWarningBanner, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(_labelWarningBanner, lv_color_white(), 0);
    lv_obj_set_style_pad_all(_labelWarningBanner, 6, 0);
    lv_obj_align(_labelWarningBanner, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
}

void DisplayManager::buildTripInfoScreen() {
    _screenTrip = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenTrip, lv_color_hex(0x0A0E14), 0);
    lv_obj_add_event_cb(_screenTrip, screenGestureCb, LV_EVENT_GESTURE, nullptr);

    lv_obj_t* title = lv_label_create(_screenTrip);
    lv_label_set_text(title, "TRIP INFO & ANALYTICS");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00D4FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    _labelTripMaxSpeed = lv_label_create(_screenTrip);
    lv_label_set_text(_labelTripMaxSpeed, "Max Speed: 0 km/h");
    lv_obj_set_style_text_color(_labelTripMaxSpeed, lv_color_white(), 0);
    lv_obj_align(_labelTripMaxSpeed, LV_ALIGN_TOP_LEFT, 20, 60);

    _labelTripAvgSpeed = lv_label_create(_screenTrip);
    lv_label_set_text(_labelTripAvgSpeed, "Avg Speed: 0 km/h");
    lv_obj_set_style_text_color(_labelTripAvgSpeed, lv_color_white(), 0);
    lv_obj_align(_labelTripAvgSpeed, LV_ALIGN_TOP_LEFT, 20, 100);

    _labelTripRideTime = lv_label_create(_screenTrip);
    lv_label_set_text(_labelTripRideTime, "Ride Time: 0m 0s");
    lv_obj_set_style_text_color(_labelTripRideTime, lv_color_white(), 0);
    lv_obj_align(_labelTripRideTime, LV_ALIGN_TOP_LEFT, 20, 140);

    _labelTripFuelRange = lv_label_create(_screenTrip);
    lv_label_set_text(_labelTripFuelRange, "Est Range: 0 km");
    lv_obj_set_style_text_color(_labelTripFuelRange, lv_color_hex(0x00E676), 0);
    lv_obj_align(_labelTripFuelRange, LV_ALIGN_TOP_LEFT, 20, 180);
}

void DisplayManager::buildNavigationScreen() {
    _screenNav = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenNav, lv_color_hex(0x0A0E14), 0);
    lv_obj_add_event_cb(_screenNav, screenGestureCb, LV_EVENT_GESTURE, nullptr);

    _labelNavTurnIcon = lv_label_create(_screenNav);
    lv_label_set_text(_labelNavTurnIcon, LV_SYMBOL_UP);
    lv_obj_set_style_text_font(_labelNavTurnIcon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(_labelNavTurnIcon, lv_color_hex(0x00D4FF), 0);
    lv_obj_align(_labelNavTurnIcon, LV_ALIGN_CENTER, -100, -20);

    _labelNavDistance = lv_label_create(_screenNav);
    lv_label_set_text(_labelNavDistance, "-- m");
    lv_obj_set_style_text_font(_labelNavDistance, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_labelNavDistance, lv_color_white(), 0);
    lv_obj_align(_labelNavDistance, LV_ALIGN_CENTER, 30, -30);

    _labelNavStreet = lv_label_create(_screenNav);
    lv_label_set_text(_labelNavStreet, "Connect Phone GPS...");
    lv_obj_set_style_text_color(_labelNavStreet, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelNavStreet, LV_ALIGN_CENTER, 0, 30);

    _labelNavEta = lv_label_create(_screenNav);
    lv_label_set_text(_labelNavEta, "ETA: --");
    lv_obj_set_style_text_color(_labelNavEta, lv_color_hex(0xFFC400), 0);
    lv_obj_align(_labelNavEta, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void DisplayManager::buildNotificationsScreen() {
    _screenNotif = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenNotif, lv_color_hex(0x0A0E14), 0);
    lv_obj_add_event_cb(_screenNotif, screenGestureCb, LV_EVENT_GESTURE, nullptr);

    lv_obj_t* title = lv_label_create(_screenNotif);
    lv_label_set_text(title, "ACTIVE WARNINGS & DIAGNOSTICS");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF1744), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    _labelNotifTitle = lv_label_create(_screenNotif);
    lv_label_set_text(_labelNotifTitle, "System Nominal");
    lv_obj_set_style_text_font(_labelNotifTitle, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_labelNotifTitle, lv_color_hex(0x00E676), 0);
    lv_obj_align(_labelNotifTitle, LV_ALIGN_CENTER, 0, -20);

    _labelNotifBody = lv_label_create(_screenNotif);
    lv_label_set_text(_labelNotifBody, "All sensors operational.");
    lv_obj_set_style_text_color(_labelNotifBody, lv_color_white(), 0);
    lv_obj_align(_labelNotifBody, LV_ALIGN_CENTER, 0, 20);
}

void DisplayManager::buildSettingsScreen() {
    _screenSettings = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenSettings, lv_color_hex(0x0A0E14), 0);
    lv_obj_add_event_cb(_screenSettings, screenGestureCb, LV_EVENT_GESTURE, nullptr);

    lv_obj_t* title = lv_label_create(_screenSettings);
    lv_label_set_text(title, "SETTINGS & PREFERENCES");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00D4FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    VehicleState cur = SharedState::instance().snapshot();

    // Speedometer Toggle
    lv_obj_t* lblSpeedo = lv_label_create(_screenSettings);
    lv_label_set_text(lblSpeedo, "Show Speedometer:");
    lv_obj_set_style_text_color(lblSpeedo, lv_color_white(), 0);
    lv_obj_align(lblSpeedo, LV_ALIGN_TOP_LEFT, 30, 60);

    _switchSpeedo = lv_switch_create(_screenSettings);
    lv_obj_align(_switchSpeedo, LV_ALIGN_TOP_RIGHT, -30, 55);
    if (cur.showSpeedometer) lv_obj_add_state(_switchSpeedo, LV_STATE_CHECKED);
    lv_obj_add_event_cb(_switchSpeedo, speedoSwitchCb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Focus Mode Toggle
    lv_obj_t* lblFocus = lv_label_create(_screenSettings);
    lv_label_set_text(lblFocus, "Minimalist Focus Mode:");
    lv_obj_set_style_text_color(lblFocus, lv_color_white(), 0);
    lv_obj_align(lblFocus, LV_ALIGN_TOP_LEFT, 30, 105);

    _switchFocus = lv_switch_create(_screenSettings);
    lv_obj_align(_switchFocus, LV_ALIGN_TOP_RIGHT, -30, 100);
    if (cur.focusMode) lv_obj_add_state(_switchFocus, LV_STATE_CHECKED);
    lv_obj_add_event_cb(_switchFocus, focusSwitchCb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Live Notification Overlay Toggle
    lv_obj_t* lblNotifOverlay = lv_label_create(_screenSettings);
    lv_label_set_text(lblNotifOverlay, "Notification Popups:");
    lv_obj_set_style_text_color(lblNotifOverlay, lv_color_white(), 0);
    lv_obj_align(lblNotifOverlay, LV_ALIGN_TOP_LEFT, 30, 150);

    _switchNotifOverlay = lv_switch_create(_screenSettings);
    lv_obj_align(_switchNotifOverlay, LV_ALIGN_TOP_RIGHT, -30, 145);
    if (cur.allowNotifOverlay) lv_obj_add_state(_switchNotifOverlay, LV_STATE_CHECKED);
    lv_obj_add_event_cb(_switchNotifOverlay, notifOverlaySwitchCb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Security Lockscreen Toggle
    lv_obj_t* lblLockscreen = lv_label_create(_screenSettings);
    lv_label_set_text(lblLockscreen, "Anti-Theft PIN Lock:");
    lv_obj_set_style_text_color(lblLockscreen, lv_color_white(), 0);
    lv_obj_align(lblLockscreen, LV_ALIGN_TOP_LEFT, 30, 195);

    _switchLockscreen = lv_switch_create(_screenSettings);
    lv_obj_align(_switchLockscreen, LV_ALIGN_TOP_RIGHT, -30, 190);
    if (cur.enableLockscreen) lv_obj_add_state(_switchLockscreen, LV_STATE_CHECKED);
    lv_obj_add_event_cb(_switchLockscreen, lockscreenSwitchCb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Profile Switch Button
    _btnProfileCycle = lv_btn_create(_screenSettings);
    lv_obj_align(_btnProfileCycle, LV_ALIGN_BOTTOM_LEFT, 30, -10);
    lv_obj_set_size(_btnProfileCycle, 200, 36);
    lv_obj_set_style_bg_color(_btnProfileCycle, lv_color_hex(0x1B2432), 0);
    lv_obj_add_event_cb(_btnProfileCycle, profileCycleBtnCb, LV_EVENT_CLICKED, nullptr);
    _labelVehicleProfile = lv_label_create(_btnProfileCycle);
    lv_label_set_text_fmt(_labelVehicleProfile, "Profile: %s", VehicleConfig::instance().active().name);
    lv_obj_set_style_text_color(_labelVehicleProfile, lv_color_hex(0x00E676), 0);
    lv_obj_center(_labelVehicleProfile);

    // OTA Wi-Fi Button
    _btnOtaWifi = lv_btn_create(_screenSettings);
    lv_obj_align(_btnOtaWifi, LV_ALIGN_BOTTOM_RIGHT, -30, -10);
    lv_obj_set_size(_btnOtaWifi, 180, 36);
    lv_obj_add_event_cb(_btnOtaWifi, otaBtnCb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* btnLbl = lv_label_create(_btnOtaWifi);
    lv_label_set_text(btnLbl, "Start OTA Wi-Fi");
    lv_obj_center(btnLbl);
}

void DisplayManager::applyTheme(ThemeMode mode) {
    lv_color_t accent = lv_color_hex(0x00D4FF);
    switch (mode) {
        case ThemeMode::SPORT:  accent = lv_color_hex(0xFF1744); break;
        case ThemeMode::NEON:   accent = lv_color_hex(0x39FF14); break;
        case ThemeMode::RETRO:  accent = lv_color_hex(0xFF8A00); break;
        default: break;
    }
    if (_arcRpm) lv_obj_set_style_arc_color(_arcRpm, accent, LV_PART_INDICATOR);
}

void DisplayManager::goToScreen(Screen s) {
    if (lvglLock(50)) {
        _currentScreen = s;
        switch (s) {
            case Screen::LOCKSCREEN:
                if (_screenLock) lv_scr_load_anim(_screenLock, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
                break;
            case Screen::MAIN_DASHBOARD:
                if (_screenMain) lv_scr_load_anim(_screenMain, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
                break;
            case Screen::TRIP_INFO:
                if (_screenTrip) lv_scr_load_anim(_screenTrip, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
                break;
            case Screen::NAVIGATION:
                if (_screenNav) lv_scr_load_anim(_screenNav, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
                break;
            case Screen::NOTIFICATIONS:
                if (_screenNotif) lv_scr_load_anim(_screenNotif, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
                break;
            case Screen::SETTINGS:
                if (_screenSettings) lv_scr_load_anim(_screenSettings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
                break;
        }
        lvglUnlock();
    }
}

void DisplayManager::updateProfileWidgets() {
    if (lvglLock(50)) {
        const auto& profile = VehicleConfig::instance().active();
        if (_labelVehicleProfile) {
            lv_label_set_text_fmt(_labelVehicleProfile, "Profile: %s", profile.name);
        }
        if (_arcRpm) {
            lv_arc_set_range(_arcRpm, 0, profile.tachoMaxRpm);
        }
        if (_barThrottle) {
            if (profile.hasThrottleBar) lv_obj_clear_flag(_barThrottle, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(_barThrottle, LV_OBJ_FLAG_HIDDEN);
        }
        if (_barEngineLoad) {
            if (profile.hasEngineLoadBar) lv_obj_clear_flag(_barEngineLoad, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(_barEngineLoad, LV_OBJ_FLAG_HIDDEN);
        }
        lvglUnlock();
    }
}

// ------------------------------------------------------- Per-frame refresh
void DisplayManager::refreshWidgetsFromState() {
    VehicleState s = SharedState::instance().snapshot();

    // 1. Refresh Main Dashboard if active
    if (_currentScreen == Screen::MAIN_DASHBOARD) {
        static int lastSpeed = -1;
        static int lastRpm = -1;
        static GearState lastGear = GearState::UNKNOWN;

        if (!s.showSpeedometer || s.focusMode) {
            lv_obj_add_flag(_labelSpeed, LV_OBJ_FLAG_HIDDEN);
            if (_labelSpeedUnit) lv_obj_add_flag(_labelSpeedUnit, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(_labelSpeed, LV_OBJ_FLAG_HIDDEN);
            if (_labelSpeedUnit) lv_obj_clear_flag(_labelSpeedUnit, LV_OBJ_FLAG_HIDDEN);
            int speedInt = (int)s.speedKmh;
            if (speedInt != lastSpeed) {
                lastSpeed = speedInt;
                lv_label_set_text_fmt(_labelSpeed, "%d", speedInt);
            }
        }

        if (s.focusMode) {
            lv_obj_add_flag(_labelTrip, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_barFuel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_labelEngineTemp, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_labelClock, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
            if (_barThrottle) lv_obj_add_flag(_barThrottle, LV_OBJ_FLAG_HIDDEN);
            if (_barEngineLoad) lv_obj_add_flag(_barEngineLoad, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(_labelTrip, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_barFuel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_labelEngineTemp, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_labelClock, LV_OBJ_FLAG_HIDDEN);
        }

        if ((int)s.rpm != lastRpm) {
            lastRpm = (int)s.rpm;
            lv_arc_set_value(_arcRpm, s.rpm);
        }

        // Dynamic gear display per transmission type & vehicle profile
        const auto& prof = VehicleConfig::instance().active();
        if (prof.transmission == TransmissionType::CVT) {
            lv_label_set_text(_labelGear, "CVT");
        } else if (prof.transmission == TransmissionType::AUTOMATIC) {
            lv_label_set_text(_labelGear, s.gearDisplay[0] != '\0' ? s.gearDisplay : "D");
        } else if (prof.hasGearIndicator) {
            if (s.gear != lastGear) {
                lastGear = s.gear;
                lv_label_set_text(_labelGear, gearToString(s.gear));
            }
        } else {
            lv_label_set_text(_labelGear, "");
        }

        // Live OBD-II telemetry bars
        if (!s.focusMode) {
            if (_barThrottle && prof.hasThrottleBar) {
                lv_obj_clear_flag(_barThrottle, LV_OBJ_FLAG_HIDDEN);
                lv_bar_set_value(_barThrottle, (int)s.throttlePct, LV_ANIM_OFF);
            }
            if (_barEngineLoad && prof.hasEngineLoadBar) {
                lv_obj_clear_flag(_barEngineLoad, LV_OBJ_FLAG_HIDDEN);
                lv_bar_set_value(_barEngineLoad, (int)s.engineLoadPct, LV_ANIM_OFF);
            }
        }

        lv_label_set_text_fmt(_labelTrip, "A %.1f km  |  ODO %.0f km", s.tripA_km, s.odometer_km);
        lv_bar_set_value(_barFuel, (int)s.fuelLevelPct, LV_ANIM_OFF);
        lv_label_set_text_fmt(_labelEngineTemp, "%.0fC", s.engineTempC);

        s.inLeftIndicator  ? lv_obj_clear_flag(_iconLeftIndicator, LV_OBJ_FLAG_HIDDEN)  : lv_obj_add_flag(_iconLeftIndicator, LV_OBJ_FLAG_HIDDEN);
        s.inRightIndicator ? lv_obj_clear_flag(_iconRightIndicator, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(_iconRightIndicator, LV_OBJ_FLAG_HIDDEN);
        s.inNeutral        ? lv_obj_clear_flag(_iconNeutral, LV_OBJ_FLAG_HIDDEN)        : lv_obj_add_flag(_iconNeutral, LV_OBJ_FLAG_HIDDEN);
        s.inHighBeam       ? lv_obj_clear_flag(_iconHighBeam, LV_OBJ_FLAG_HIDDEN)       : lv_obj_add_flag(_iconHighBeam, LV_OBJ_FLAG_HIDDEN);

        const Notification* n = NotificationManager::instance().current();
        if (n && s.allowNotifOverlay && !s.focusMode) {
            lv_label_set_text(_labelWarningBanner, n->title.c_str());
            lv_obj_clear_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
        }

        static int lastMin = -1;
        time_t now = time(nullptr);
        struct tm* tmv = localtime(&now);
        if (tmv && tmv->tm_min != lastMin) {
            lastMin = tmv->tm_min;
            lv_label_set_text_fmt(_labelClock, "%02d:%02d", tmv->tm_hour, tmv->tm_min);
        }
    }
    // 2. Refresh Trip Screen if active
    else if (_currentScreen == Screen::TRIP_INFO) {
        if (_labelTripMaxSpeed) lv_label_set_text_fmt(_labelTripMaxSpeed, "Max Speed: %.0f km/h", s.maxSpeedKmh);
        if (_labelTripAvgSpeed) lv_label_set_text_fmt(_labelTripAvgSpeed, "Avg Speed: %.0f km/h", s.avgSpeedKmh);
        if (_labelTripRideTime) lv_label_set_text_fmt(_labelTripRideTime, "Ride Time: %lum %lus", s.rideTimerSec / 60, s.rideTimerSec % 60);
        if (_labelTripFuelRange) lv_label_set_text_fmt(_labelTripFuelRange, "Est Range: %.0f km", s.fuelRangeKm);
    }
    // 3. Refresh Navigation Screen if active
    else if (_currentScreen == Screen::NAVIGATION) {
        if (_labelNavTurnIcon) {
            const char* icon = LV_SYMBOL_UP;
            if (s.navTurnIcon == 1) icon = LV_SYMBOL_LEFT;
            else if (s.navTurnIcon == 2) icon = LV_SYMBOL_RIGHT;
            else if (s.navTurnIcon == 3) icon = LV_SYMBOL_LOOP;
            lv_label_set_text(_labelNavTurnIcon, icon);
        }
        if (_labelNavDistance) {
            if (s.navDistanceMeters >= 1000) {
                lv_label_set_text_fmt(_labelNavDistance, "%.1f km", s.navDistanceMeters / 1000.0f);
            } else {
                lv_label_set_text_fmt(_labelNavDistance, "%lu m", s.navDistanceMeters);
            }
        }
        if (_labelNavStreet) {
            lv_label_set_text(_labelNavStreet, s.navStreetName[0] ? s.navStreetName : "Continue on Route");
        }
        if (_labelNavEta) {
            lv_label_set_text_fmt(_labelNavEta, "ETA: %u min", s.navEtaMinutes);
        }
    }
    // 4. Refresh Notifications Screen if active
    else if (_currentScreen == Screen::NOTIFICATIONS) {
        const Notification* curNotif = NotificationManager::instance().current();
        if (curNotif) {
            if (_labelNotifTitle) {
                lv_label_set_text(_labelNotifTitle, curNotif->title.c_str());
                lv_obj_set_style_text_color(_labelNotifTitle, (curNotif->priority == NotifPriority::CRITICAL) ? lv_color_hex(0xFF1744) : lv_color_hex(0xFFC400), 0);
            }
            if (_labelNotifBody) lv_label_set_text(_labelNotifBody, curNotif->body.c_str());
        } else {
            if (_labelNotifTitle) {
                lv_label_set_text(_labelNotifTitle, "System Nominal");
                lv_obj_set_style_text_color(_labelNotifTitle, lv_color_hex(0x00E676), 0);
            }
            if (_labelNotifBody) lv_label_set_text(_labelNotifBody, "All sensors operational.");
        }
    }
}

void DisplayManager::showRideSummaryScreen() {
    VehicleState s = SharedState::instance().snapshot();
    lv_obj_t* summary = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(summary, lv_color_hex(0x0A0E14), 0);

    lv_obj_t* title = lv_label_create(summary);
    lv_label_set_text(title, "TRIP COMPLETED - RIDE SUMMARY");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00D4FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t* stats = lv_label_create(summary);
    lv_label_set_text_fmt(stats, "Distance: %.1f km\nMax Speed: %.0f km/h\nAvg Speed: %.0f km/h\nRide Time: %lum %lus",
                          s.tripA_km, s.maxSpeedKmh, s.avgSpeedKmh, s.rideTimerSec / 60, s.rideTimerSec % 60);
    lv_obj_set_style_text_color(stats, lv_color_white(), 0);
    lv_obj_align(stats, LV_ALIGN_CENTER, 0, 0);

    lv_scr_load(summary);
    lv_timer_handler();
    delay(2000);   // Show summary for 2 seconds before goodbye screen
    lv_obj_del(summary);
}

void DisplayManager::showGoodbyeScreen() {
    lv_obj_t* goodbye = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(goodbye, lv_color_black(), 0);
    lv_obj_t* label = lv_label_create(goodbye);
    lv_label_set_text(label, "See you next ride");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
    lv_scr_load(goodbye);
    lv_timer_handler();
    delay(1200);   // synchronous — this only runs during the shutdown path
}
