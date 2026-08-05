#include "DisplayManager.h"
#include "Config.h"
#include "NotificationManager.h"
#include <TFT_eSPI.h>

static TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t drawBuf;
static lv_color_t buf1[SCREEN_W * 40];   // partial buffer (40 lines) — keeps RAM use sane on PSRAM-less parts
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

// ---------------------------------------------------------------------------
void DisplayManager::begin() {
    tft.init();
    tft.setRotation(1);   // landscape
    ledcSetup(0, 5000, 8);
    ledcAttachPin(PIN_TFT_BL, 0);
    ledcWrite(0, 200);    // initial backlight ~78%, refined by auto-brightness later

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
}

void DisplayManager::playStartupSweepAnimation() {
    if (!_arcRpm) return;
    // Motorcycle Gauge Sweep: 0 -> 12,000 RPM (Redline limit) -> 0 RPM
    for (int rpm = 0; rpm <= 12000; rpm += 400) {
        lv_arc_set_value(_arcRpm, rpm);
        lv_timer_handler();
        delay(15);
    }
    delay(100); // Redline pause
    for (int rpm = 12000; rpm >= 0; rpm -= 400) {
        lv_arc_set_value(_arcRpm, rpm);
        lv_timer_handler();
        delay(15);
    }
}

static void drawerItemClickCb(lv_event_t* e) {
    Screen targetScreen = (Screen)(uintptr_t)lv_event_get_user_data(e);
    DisplayManager::instance().goToScreen(targetScreen);
    DisplayManager::instance().toggleMenuDrawer();
}

void DisplayManager::buildMenuDrawer() {
    _drawerContainer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_drawerContainer, 200, 320);
    lv_obj_align(_drawerContainer, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(_drawerContainer, lv_color_hex(0x0A0E14), 0);
    lv_obj_set_style_bg_opa(_drawerContainer, LV_OPA_90, 0);
    lv_obj_set_style_border_color(_drawerContainer, lv_color_hex(0x00D4FF), 0);
    lv_obj_set_style_border_width(_drawerContainer, 2, 0);
    lv_obj_set_flex_flow(_drawerContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_drawerContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* title = lv_label_create(_drawerContainer);
    lv_label_set_text(title, "COCKPIT MENU");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00D4FF), 0);

    struct MenuItem { const char* name; Screen screen; };
    MenuItem items[] = {
        {"Main Dashboard", Screen::MAIN_DASHBOARD},
        {"Trip Analytics", Screen::TRIP_INFO},
        {"Turn Navigation", Screen::NAVIGATION},
        {"Notifications", Screen::NOTIFICATIONS},
        {"Settings & Calib", Screen::SETTINGS}
    };

    for (int i = 0; i < 5; i++) {
        lv_obj_t* btn = lv_btn_create(_drawerContainer);
        lv_obj_set_size(btn, 170, 38);
        lv_obj_add_event_cb(btn, drawerItemClickCb, LV_EVENT_CLICKED, (void*)(uintptr_t)items[i].screen);
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, items[i].name);
        lv_obj_center(lbl);
    }

    lv_obj_add_flag(_drawerContainer, LV_OBJ_FLAG_HIDDEN);
}

void DisplayManager::toggleMenuDrawer() {
    _isDrawerOpen = !_isDrawerOpen;
    if (_isDrawerOpen) {
        lv_obj_clear_flag(_drawerContainer, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_drawerContainer, LV_OBJ_FLAG_HIDDEN);
    }
}

void DisplayManager::buildLockScreen() {
    _screenLock = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenLock, lv_color_hex(0x07090E), 0);

    lv_obj_t* icon = lv_label_create(_screenLock);
    lv_label_set_text(icon, "[ IMMOBILIZED ]");
    lv_obj_set_style_text_color(icon, lv_color_hex(0xFF1744), 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -80);

    _labelLockStatus = lv_label_create(_screenLock);
    lv_label_set_text(_labelLockStatus, "VEHICLE IMMOBILIZED");
    lv_obj_set_style_text_font(_labelLockStatus, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_labelLockStatus, lv_color_white(), 0);
    lv_obj_align(_labelLockStatus, LV_ALIGN_CENTER, 0, -20);

    _labelLockPin = lv_label_create(_screenLock);
    lv_label_set_text(_labelLockPin, "Enter 4-Digit Security PIN or Connect Smartphone");
    lv_obj_set_style_text_color(_labelLockPin, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelLockPin, LV_ALIGN_CENTER, 0, 30);

    lv_obj_t* subText = lv_label_create(_screenLock);
    lv_label_set_text(subText, "BLE Keyless Proximity Auto-Unlock Active");
    lv_obj_set_style_text_color(subText, lv_color_hex(0x00D4FF), 0);
    lv_obj_align(subText, LV_ALIGN_BOTTOM_MID, 0, -15);
}

void DisplayManager::taskEntry(void* pv) {
    DisplayManager& self = instance();
    const TickType_t period = pdMS_TO_TICKS(1000 / TARGET_FPS);
    TickType_t lastWake = xTaskGetTickCount();
    for (;;) {
        self.tick();
        vTaskDelayUntil(&lastWake, period);
    }
}

void DisplayManager::tick() {
    lv_timer_handler();          // let LVGL process animations/input
    refreshWidgetsFromState();   // push latest VehicleState into widgets
}

static void screenGestureCb(lv_event_t* e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_LEFT) {
        DisplayManager::instance().toggleMenuDrawer();
    }
}

// ------------------------------------------------------------ Screen build
void DisplayManager::buildMainDashboard() {
    _screenMain = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenMain, lv_color_hex(0x0A0E14), 0);
    lv_obj_add_event_cb(_screenMain, screenGestureCb, LV_EVENT_GESTURE, nullptr);

    // --- Speed (large, center) ---
    _labelSpeed = lv_label_create(_screenMain);
    lv_obj_set_style_text_font(_labelSpeed, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(_labelSpeed, lv_color_white(), 0);
    lv_obj_align(_labelSpeed, LV_ALIGN_CENTER, 0, -10);
    lv_label_set_text(_labelSpeed, "0");

    _labelSpeedUnit = lv_label_create(_screenMain);
    lv_obj_set_style_text_color(_labelSpeedUnit, lv_color_hex(0x8A93A8), 0);
    lv_obj_align_to(_labelSpeedUnit, _labelSpeed, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
    lv_label_set_text(_labelSpeedUnit, "km/h");

    // --- RPM arc (surrounds speed, OEM-style sweep gauge) ---
    _arcRpm = lv_arc_create(_screenMain);
    lv_obj_set_size(_arcRpm, 260, 260);
    lv_arc_set_rotation(_arcRpm, 135);
    lv_arc_set_bg_angles(_arcRpm, 0, 270);
    lv_arc_set_range(_arcRpm, 0, 12000);   // Avenger 150 redline ~ 9-10k, headroom to 12k
    lv_obj_align(_arcRpm, LV_ALIGN_CENTER, 0, -10);
    lv_obj_remove_style(_arcRpm, nullptr, LV_PART_KNOB);   // no draggable knob — display only
    lv_obj_clear_flag(_arcRpm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(_arcRpm, lv_color_hex(0x00D4FF), LV_PART_INDICATOR);

    // --- Gear indicator ---
    _labelGear = lv_label_create(_screenMain);
    lv_obj_set_style_text_font(_labelGear, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_labelGear, lv_color_hex(0xFFC400), 0);
    lv_obj_align(_labelGear, LV_ALIGN_CENTER, 0, 55);
    lv_label_set_text(_labelGear, "N");

    // --- Clock (top-right) ---
    _labelClock = lv_label_create(_screenMain);
    lv_obj_set_style_text_color(_labelClock, lv_color_white(), 0);
    lv_obj_align(_labelClock, LV_ALIGN_TOP_RIGHT, -10, 8);
    lv_label_set_text(_labelClock, "--:--");

    // --- Trip / odometer (bottom) ---
    _labelTrip = lv_label_create(_screenMain);
    lv_obj_set_style_text_color(_labelTrip, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelTrip, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_label_set_text(_labelTrip, "A 0.0 km");

    // --- Fuel bar (left) ---
    _barFuel = lv_bar_create(_screenMain);
    lv_obj_set_size(_barFuel, 16, 140);
    lv_obj_align(_barFuel, LV_ALIGN_LEFT_MID, 12, 0);
    lv_bar_set_range(_barFuel, 0, 100);
    lv_obj_set_style_bg_color(_barFuel, lv_color_hex(0x00E676), LV_PART_INDICATOR);

    // --- Engine temp (right) ---
    _labelEngineTemp = lv_label_create(_screenMain);
    lv_obj_set_style_text_color(_labelEngineTemp, lv_color_white(), 0);
    lv_obj_align(_labelEngineTemp, LV_ALIGN_RIGHT_MID, -12, -40);
    lv_label_set_text(_labelEngineTemp, "--C");

    // --- Indicator icon row (top) ---
    _iconLeftIndicator = lv_label_create(_screenMain);
    lv_label_set_text(_iconLeftIndicator, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(_iconLeftIndicator, lv_color_hex(0x00E676), 0);
    lv_obj_align(_iconLeftIndicator, LV_ALIGN_TOP_LEFT, 10, 8);
    lv_obj_add_flag(_iconLeftIndicator, LV_OBJ_FLAG_HIDDEN);

    _iconRightIndicator = lv_label_create(_screenMain);
    lv_label_set_text(_iconRightIndicator, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(_iconRightIndicator, lv_color_hex(0x00E676), 0);
    lv_obj_align(_iconRightIndicator, LV_ALIGN_TOP_LEFT, 40, 8);
    lv_obj_add_flag(_iconRightIndicator, LV_OBJ_FLAG_HIDDEN);

    _iconNeutral = lv_label_create(_screenMain);
    lv_label_set_text(_iconNeutral, "N");
    lv_obj_set_style_text_color(_iconNeutral, lv_color_hex(0x00D4FF), 0);
    lv_obj_align(_iconNeutral, LV_ALIGN_TOP_MID, -30, 8);
    lv_obj_add_flag(_iconNeutral, LV_OBJ_FLAG_HIDDEN);

    _iconHighBeam = lv_label_create(_screenMain);
    lv_label_set_text(_iconHighBeam, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_color(_iconHighBeam, lv_color_hex(0x2979FF), 0);
    lv_obj_align(_iconHighBeam, LV_ALIGN_TOP_MID, 30, 8);
    lv_obj_add_flag(_iconHighBeam, LV_OBJ_FLAG_HIDDEN);

    // --- Warning banner (hidden until a notification is active) ---
    _labelWarningBanner = lv_label_create(_screenMain);
    lv_obj_set_style_bg_color(_labelWarningBanner, lv_color_hex(0xFF1744), 0);
    lv_obj_set_style_bg_opa(_labelWarningBanner, LV_OPA_80, 0);
    lv_obj_set_style_text_color(_labelWarningBanner, lv_color_white(), 0);
    lv_obj_set_style_pad_all(_labelWarningBanner, 6, 0);
    lv_obj_align(_labelWarningBanner, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
}

void DisplayManager::buildTripInfoScreen() {
    _screenTrip = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenTrip, lv_color_hex(0x0A0E14), 0);

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

    lv_obj_t* title = lv_label_create(_screenSettings);
    lv_label_set_text(title, "SYSTEM SETTINGS & TOGGLES");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00D4FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Speedometer Toggle
    lv_obj_t* lblSpeedo = lv_label_create(_screenSettings);
    lv_label_set_text(lblSpeedo, "Show Speedometer:");
    lv_obj_set_style_text_color(lblSpeedo, lv_color_white(), 0);
    lv_obj_align(lblSpeedo, LV_ALIGN_TOP_LEFT, 30, 70);

    _switchSpeedo = lv_switch_create(_screenSettings);
    lv_obj_align(_switchSpeedo, LV_ALIGN_TOP_RIGHT, -30, 65);
    lv_obj_add_state(_switchSpeedo, LV_STATE_CHECKED);

    // Focus Mode Toggle
    lv_obj_t* lblFocus = lv_label_create(_screenSettings);
    lv_label_set_text(lblFocus, "Minimalist Focus Mode:");
    lv_obj_set_style_text_color(lblFocus, lv_color_white(), 0);
    lv_obj_align(lblFocus, LV_ALIGN_TOP_LEFT, 30, 120);

    _switchFocus = lv_switch_create(_screenSettings);
    lv_obj_align(_switchFocus, LV_ALIGN_TOP_RIGHT, -30, 115);

    // Live Notification Overlay Toggle
    lv_obj_t* lblNotifOverlay = lv_label_create(_screenSettings);
    lv_label_set_text(lblNotifOverlay, "Live Notif Overlay On Main Display:");
    lv_obj_set_style_text_color(lblNotifOverlay, lv_color_white(), 0);
    lv_obj_align(lblNotifOverlay, LV_ALIGN_TOP_LEFT, 30, 170);

    lv_obj_t* switchNotifOverlay = lv_switch_create(_screenSettings);
    lv_obj_align(switchNotifOverlay, LV_ALIGN_TOP_RIGHT, -30, 165);
    lv_obj_add_state(switchNotifOverlay, LV_STATE_CHECKED);

    // Security Lockscreen Toggle
    lv_obj_t* lblLockscreen = lv_label_create(_screenSettings);
    lv_label_set_text(lblLockscreen, "Enable Anti-Theft Security Lockscreen:");
    lv_obj_set_style_text_color(lblLockscreen, lv_color_white(), 0);
    lv_obj_align(lblLockscreen, LV_ALIGN_TOP_LEFT, 30, 220);

    lv_obj_t* switchLockscreen = lv_switch_create(_screenSettings);
    lv_obj_align(switchLockscreen, LV_ALIGN_TOP_RIGHT, -30, 215);
    lv_obj_add_state(switchLockscreen, LV_STATE_CHECKED);

    // OTA Wi-Fi Button
    _btnOtaWifi = lv_btn_create(_screenSettings);
    lv_obj_align(_btnOtaWifi, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_size(_btnOtaWifi, 220, 36);
    lv_obj_t* btnLbl = lv_label_create(_btnOtaWifi);
    lv_label_set_text(btnLbl, "Start Wireless OTA AP");
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
}

// ------------------------------------------------------- Per-frame refresh
void DisplayManager::refreshWidgetsFromState() {
    VehicleState s = SharedState::instance().snapshot();

    static int lastSpeed = -1;
    static int lastRpm = -1;
    static GearState lastGear = GearState::UNKNOWN;

    // Speedometer Toggle (hide numeric speed if rider has an OEM speedometer)
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

    // Pure Tachometer Focus Mode (strips all distractions except giant Tach Arc & Gear)
    if (s.focusMode) {
        lv_obj_add_flag(_labelTrip, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_barFuel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_labelEngineTemp, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_labelClock, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
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

    if (s.gear != lastGear) {
        lastGear = s.gear;
        const char* gearStr = "N";
        switch (s.gear) {
            case GearState::GEAR_1: gearStr = "1"; break;
            case GearState::GEAR_2: gearStr = "2"; break;
            case GearState::GEAR_3: gearStr = "3"; break;
            case GearState::GEAR_4: gearStr = "4"; break;
            case GearState::GEAR_5: gearStr = "5"; break;
            case GearState::NEUTRAL: gearStr = "N"; break;
            default: gearStr = "-"; break;
        }
        lv_label_set_text(_labelGear, gearStr);
    }

    lv_label_set_text_fmt(_labelTrip, "A %.1f km  |  ODO %.0f km", s.tripA_km, s.odometer_km);
    lv_bar_set_value(_barFuel, (int)s.fuelLevelPct, LV_ANIM_ON);
    lv_label_set_text_fmt(_labelEngineTemp, "%.0fC", s.engineTempC);

    s.inLeftIndicator  ? lv_obj_clear_flag(_iconLeftIndicator, LV_OBJ_FLAG_HIDDEN)  : lv_obj_add_flag(_iconLeftIndicator, LV_OBJ_FLAG_HIDDEN);
    s.inRightIndicator ? lv_obj_clear_flag(_iconRightIndicator, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(_iconRightIndicator, LV_OBJ_FLAG_HIDDEN);
    s.inNeutral          ? lv_obj_clear_flag(_iconNeutral, LV_OBJ_FLAG_HIDDEN)          : lv_obj_add_flag(_iconNeutral, LV_OBJ_FLAG_HIDDEN);
    s.inHighBeam          ? lv_obj_clear_flag(_iconHighBeam, LV_OBJ_FLAG_HIDDEN)          : lv_obj_add_flag(_iconHighBeam, LV_OBJ_FLAG_HIDDEN);

    // Warning / Phone Notification Banner (shown on main display if allowNotifOverlay is enabled)
    const Notification* n = NotificationManager::instance().current();
    if (n && s.allowNotifOverlay && !s.focusMode) {
        lv_label_set_text(_labelWarningBanner, n->title.c_str());
        lv_obj_clear_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
    }

    // Clock — throttled to update only when minute changes (saves CPU cycles per frame)
    static int lastMin = -1;
    time_t now = time(nullptr);
    struct tm* tmv = localtime(&now);
    if (tmv && tmv->tm_min != lastMin) {
        lastMin = tmv->tm_min;
        lv_label_set_text_fmt(_labelClock, "%02d:%02d", tmv->tm_hour, tmv->tm_min);
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
    lv_label_set_text_fmt(stats, "Distance: %.1f km\nMax Speed: %.0f km/h\nAvg Speed: %.0f km/h\nRide Time: %dm %ds",
                          s.tripA_km, s.maxSpeedKmh, s.avgSpeedKmh, s.rideTimerSec / 60, s.rideTimerSec % 60);
    lv_obj_set_style_text_color(stats, lv_color_white(), 0);
    lv_obj_align(stats, LV_ALIGN_CENTER, 0, 0);

    lv_scr_load(summary);
    lv_timer_handler();
    delay(2000);   // Show summary for 2 seconds before goodbye screen
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
