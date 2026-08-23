#include "DisplayManager.h"
#include "Config.h"
#include "NotificationManager.h"
#include "RideManager.h"
#include "StorageManager.h"
#include "RideModeProfile.h"
#include "MaintenanceManager.h"
#include "PhoneLinkManager.h"
#include "DisplayPolicyMath.h"
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

    pinMode(PIN_BTN_MODE, INPUT_PULLUP);
    pinMode(PIN_BTN_OK, INPUT_PULLUP);
    pinMode(PIN_ROTARY_SW, INPUT_PULLUP);
    // PIN_ROTARY_A/B quadrature decode isn't implemented yet — rotary press
    // (PIN_ROTARY_SW) is wired as an alias for the OK button below, but
    // turning the encoder doesn't yet navigate. Touch swipe + the MODE
    // button already cover full navigation; full quadrature decode is a
    // straightforward follow-up (attach interrupts on A/B, same pattern as
    // SensorManager's pulse counters) once the rest of this is validated.

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

    buildMainDashboard();
    buildTripInfoScreen();
    buildNotificationsScreen();
    buildSettingsScreen();
    buildMusicWidget();
    buildNavBanner();
    buildCallModal();   // built last — it's a top-layer overlay, independent of the screen stack below it

    // Restore last-saved theme/ride-mode rather than always booting into
    // the hardcoded defaults — see docs/nvs_layout.md for the `theme`/
    // `ride_mode` keys this reads.
    _selectedTheme = (ThemeMode)StorageManager::instance().loadTheme((uint8_t)ThemeMode::MODERN_DIGITAL);
    RideMode savedMode = (RideMode)StorageManager::instance().loadRideMode((uint8_t)RideMode::CITY);
    applyTheme(_selectedTheme);   // restored theme takes priority over the mode's default on boot
    applyRideModeProfile(savedMode, /*persist=*/false);   // brightness cap + VehicleState, no re-save/no advisory
    refreshSettingsLabels();

    goToScreen(Screen::MAIN_DASHBOARD);
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
    refreshPhoneLinkWidgets();   // call modal / music widget / nav banner
    handlePhysicalInputs();

    // A CRITICAL notification force-switches to the Notifications screen
    // and stays there, regardless of what screen the rider was on — see
    // docs/screen_flow.md "Transition rules". nextScreen()/prevScreen()
    // independently refuse to navigate away while this holds, so touch
    // swipes can't escape it either. An incoming call takes priority over
    // even that — it's drawn on lv_layer_top() so it's visible regardless
    // of which screen is active underneath, no screen switch needed.
    const Notification* n = NotificationManager::instance().current();
    if (n && n->priority == NotifPriority::CRITICAL && _currentScreen != Screen::NOTIFICATIONS) {
        goToScreen(Screen::NOTIFICATIONS);
    }
}

// --------------------------------------------------------------- Navigation
void DisplayManager::gestureEventCb(lv_event_t* e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    DisplayManager* self = (DisplayManager*)lv_event_get_user_data(e);
    if (dir == LV_DIR_LEFT) self->nextScreen();
    else if (dir == LV_DIR_RIGHT) self->prevScreen();
}

lv_obj_t* DisplayManager::attachGestureNav(lv_obj_t* screen) {
    lv_obj_add_event_cb(screen, gestureEventCb, LV_EVENT_GESTURE, &instance());
    return screen;
}

void DisplayManager::nextScreen() {
    const Notification* n = NotificationManager::instance().current();
    if (n && n->priority == NotifPriority::CRITICAL) return;   // blocked, see tick()

    switch (_currentScreen) {
        case Screen::MAIN_DASHBOARD:  goToScreen(Screen::TRIP_INFO); break;
        case Screen::TRIP_INFO:        goToScreen(Screen::NOTIFICATIONS); break;
        case Screen::NOTIFICATIONS:     goToScreen(Screen::SETTINGS); break;
        case Screen::SETTINGS:           goToScreen(Screen::MAIN_DASHBOARD); break;
    }
}

void DisplayManager::prevScreen() {
    const Notification* n = NotificationManager::instance().current();
    if (n && n->priority == NotifPriority::CRITICAL) return;

    switch (_currentScreen) {
        case Screen::MAIN_DASHBOARD:  goToScreen(Screen::SETTINGS); break;
        case Screen::TRIP_INFO:        goToScreen(Screen::MAIN_DASHBOARD); break;
        case Screen::NOTIFICATIONS:     goToScreen(Screen::TRIP_INFO); break;
        case Screen::SETTINGS:           goToScreen(Screen::NOTIFICATIONS); break;
    }
}

void DisplayManager::handlePhysicalInputs() {
    bool modeBtn = digitalRead(PIN_BTN_MODE) == LOW;    // active-low (INPUT_PULLUP)
    bool okBtn = (digitalRead(PIN_BTN_OK) == LOW) || (digitalRead(PIN_ROTARY_SW) == LOW);

    // MODE button: short press = next screen, long press (>1s) = jump to
    // Settings from anywhere (per docs/screen_flow.md navigation table).
    if (modeBtn && !_lastModeBtn) {
        _modeBtnDownMs = millis();
        _modeLongPressFired = false;
    }
    if (modeBtn && !_modeLongPressFired && (millis() - _modeBtnDownMs > LONG_PRESS_MS)) {
        _modeLongPressFired = true;
        goToScreen(Screen::SETTINGS);
    }
    if (!modeBtn && _lastModeBtn && !_modeLongPressFired) {
        nextScreen();   // released before the long-press threshold -> short press
    }
    _lastModeBtn = modeBtn;

    // OK button / rotary press: acknowledges the current notification when
    // on the Notifications screen; reserved for Settings row-select
    // elsewhere (touch already covers Settings interaction in this build).
    if (okBtn && !_lastOkBtn && _currentScreen == Screen::NOTIFICATIONS) {
        NotificationManager::instance().acknowledgeCurrent();
        refreshNotificationsList();
    }
    _lastOkBtn = okBtn;
}

// ------------------------------------------------------------ Screen build
void DisplayManager::buildMainDashboard() {
    _screenMain = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenMain, lv_color_hex(0x0A0E14), 0);
    attachGestureNav(_screenMain);

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

void DisplayManager::buildMusicWidget() {
    // Lives on the Main Dashboard only, bottom-left — small enough not to
    // compete with speed/RPM, hidden by default until PhoneLinkManager has
    // fresh music data (see refreshPhoneLinkWidgets()).
    _musicWidget = lv_obj_create(_screenMain);
    lv_obj_set_size(_musicWidget, 180, 44);
    lv_obj_align(_musicWidget, LV_ALIGN_BOTTOM_LEFT, 8, -36);
    lv_obj_set_style_bg_color(_musicWidget, lv_color_hex(0x1A2130), 0);
    lv_obj_set_style_bg_opa(_musicWidget, LV_OPA_70, 0);
    lv_obj_set_style_border_width(_musicWidget, 0, 0);
    lv_obj_set_style_pad_all(_musicWidget, 4, 0);
    lv_obj_clear_flag(_musicWidget, LV_OBJ_FLAG_SCROLLABLE);

    _labelMusicTrack = lv_label_create(_musicWidget);
    lv_obj_set_style_text_color(_labelMusicTrack, lv_color_white(), 0);
    lv_obj_set_style_text_font(_labelMusicTrack, &lv_font_montserrat_12, 0);
    lv_obj_set_width(_labelMusicTrack, 108);
    lv_label_set_long_mode(_labelMusicTrack, LV_LABEL_LONG_SCROLL_CIRCULAR);   // marquee for long titles
    lv_obj_align(_labelMusicTrack, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(_labelMusicTrack, "");

    _btnMusicPlayPause = lv_btn_create(_musicWidget);
    lv_obj_set_size(_btnMusicPlayPause, 32, 32);
    lv_obj_align(_btnMusicPlayPause, LV_ALIGN_RIGHT_MID, -36, 0);
    lv_obj_add_event_cb(_btnMusicPlayPause, onMusicPlayPauseClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblPlayPause = lv_label_create(_btnMusicPlayPause);
    lv_label_set_text(lblPlayPause, LV_SYMBOL_PLAY "/" LV_SYMBOL_PAUSE);
    lv_obj_set_style_text_font(lblPlayPause, &lv_font_montserrat_12, 0);
    lv_obj_center(lblPlayPause);

    lv_obj_t* btnNext = lv_btn_create(_musicWidget);
    lv_obj_set_size(btnNext, 32, 32);
    lv_obj_align(btnNext, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(btnNext, onMusicNextClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblNext = lv_label_create(btnNext);
    lv_label_set_text(lblNext, LV_SYMBOL_NEXT);
    lv_obj_center(lblNext);

    // Deliberately no "previous track" button — three small touch targets
    // in 180px is already tight on a gloved hand; skip/play-pause covers
    // the actual "change what's playing without grabbing the phone" need.
    // requestMusicPrev() exists in PhoneLinkManager for a companion app
    // that wants to offer it in its own UI even though the dash doesn't.

    lv_obj_add_flag(_musicWidget, LV_OBJ_FLAG_HIDDEN);   // shown once fresh data arrives
}

void DisplayManager::buildNavBanner() {
    // Compact bar just under the top edge of Main Dashboard — a next-turn
    // relay from the phone's own nav app, not on-device routing. Hidden by
    // default; see refreshPhoneLinkWidgets() for the staleness-gated show/hide.
    _navBanner = lv_obj_create(_screenMain);
    lv_obj_set_size(_navBanner, 260, 32);
    lv_obj_align(_navBanner, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(_navBanner, lv_color_hex(0x0D3B66), 0);
    lv_obj_set_style_bg_opa(_navBanner, LV_OPA_80, 0);
    lv_obj_set_style_border_width(_navBanner, 0, 0);
    lv_obj_set_style_pad_all(_navBanner, 4, 0);
    lv_obj_clear_flag(_navBanner, LV_OBJ_FLAG_SCROLLABLE);

    _labelNavInstruction = lv_label_create(_navBanner);
    lv_obj_set_style_text_color(_labelNavInstruction, lv_color_white(), 0);
    lv_obj_center(_labelNavInstruction);
    lv_label_set_text(_labelNavInstruction, "");

    lv_obj_add_flag(_navBanner, LV_OBJ_FLAG_HIDDEN);

    // Note: this sits at the same TOP_MID anchor the warning banner uses
    // (offset differently, y=8 vs y=40) — a CRITICAL warning and an active
    // nav hint can both be visible at once without overlapping. Two
    // non-critical banners competing for attention at once (nav + a
    // quiet-mode-suppressed notification) doesn't happen by construction:
    // quiet mode only suppresses the warning banner above
    // QUIET_MODE_SPEED_THRESHOLD_KMH, exactly when you'd also most want
    // the nav banner uncluttered.
}

void DisplayManager::buildCallModal() {
    // Top-layer overlay (lv_layer_top()) so it appears above whichever
    // screen is currently active without needing every screen to know
    // about it — the same reason LVGL's own system keyboard/message-box
    // widgets use this layer.
    _callModal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_callModal, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(_callModal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_callModal, LV_OPA_90, 0);
    lv_obj_center(_callModal);
    lv_obj_clear_flag(_callModal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lblIncoming = lv_label_create(_callModal);
    lv_label_set_text(lblIncoming, "Incoming Call");
    lv_obj_set_style_text_color(lblIncoming, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(lblIncoming, LV_ALIGN_TOP_MID, 0, 60);

    _labelCallerName = lv_label_create(_callModal);
    lv_obj_set_style_text_font(_labelCallerName, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_labelCallerName, lv_color_white(), 0);
    lv_obj_align(_labelCallerName, LV_ALIGN_CENTER, 0, -20);
    lv_label_set_text(_labelCallerName, "");

    lv_obj_t* btnReject = lv_btn_create(_callModal);
    lv_obj_set_size(btnReject, 100, 50);
    lv_obj_align(btnReject, LV_ALIGN_BOTTOM_MID, -70, -50);
    lv_obj_set_style_bg_color(btnReject, lv_color_hex(0xFF1744), 0);
    lv_obj_add_event_cb(btnReject, onCallRejectClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblReject = lv_label_create(btnReject);
    lv_label_set_text(lblReject, LV_SYMBOL_CLOSE " Reject");
    lv_obj_center(lblReject);

    lv_obj_t* btnAccept = lv_btn_create(_callModal);
    lv_obj_set_size(btnAccept, 100, 50);
    lv_obj_align(btnAccept, LV_ALIGN_BOTTOM_MID, 70, -50);
    lv_obj_set_style_bg_color(btnAccept, lv_color_hex(0x00E676), 0);
    lv_obj_add_event_cb(btnAccept, onCallAcceptClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblAccept = lv_label_create(btnAccept);
    lv_label_set_text(lblAccept, LV_SYMBOL_CALL " Accept");
    lv_obj_center(lblAccept);

    lv_obj_add_flag(_callModal, LV_OBJ_FLAG_HIDDEN);
    // Deliberately NOT wired into the swipe/MODE-button navigation cycle —
    // an incoming call isn't a "screen" you navigate to, it's an interrupt
    // that appears over whatever you were doing and goes away on
    // accept/reject/timeout. See docs/phone_link.md.
}

void DisplayManager::buildTripInfoScreen() {
    _screenTripInfo = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenTripInfo, lv_color_hex(0x0A0E14), 0);
    attachGestureNav(_screenTripInfo);

    lv_obj_t* title = lv_label_create(_screenTripInfo);
    lv_label_set_text(title, "Trip Info");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    // Two-column layout: left column = trip/odo/timer, right column = fuel/speed
    auto makeRow = [&](int y, const char* labelTxt) -> lv_obj_t* {
        lv_obj_t* row = lv_label_create(_screenTripInfo);
        lv_obj_set_style_text_color(row, lv_color_white(), 0);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 16, y);
        lv_label_set_text(row, labelTxt);
        return row;
    };

    _labelTripA = makeRow(50, "Trip A: 0.0 km");
    _labelTripB = makeRow(80, "Trip B: 0.0 km");
    _labelOdometer = makeRow(110, "Odometer: 0 km");
    _labelRideTimer = makeRow(140, "Ride Time: 00:00");
    _labelAvgSpeed = makeRow(180, "Avg Speed: 0 km/h");
    _labelMaxSpeed = makeRow(210, "Max Speed: 0 km/h");
    _labelFuelRange = makeRow(240, "Range: -- km");
    _labelFuelConsumption = makeRow(270, "Efficiency: -- km/L");

    // Reset buttons — deliberately require a touch tap on the actual
    // dashboard (not a remote/BLE command) so a trip reset always reflects
    // something the rider did in front of the bike.
    lv_obj_t* btnResetA = lv_btn_create(_screenTripInfo);
    lv_obj_set_size(btnResetA, 100, 40);
    lv_obj_align(btnResetA, LV_ALIGN_BOTTOM_LEFT, 20, -16);
    lv_obj_add_event_cb(btnResetA, onResetTripAClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblResetA = lv_label_create(btnResetA);
    lv_label_set_text(lblResetA, "Reset A");
    lv_obj_center(lblResetA);

    lv_obj_t* btnResetB = lv_btn_create(_screenTripInfo);
    lv_obj_set_size(btnResetB, 100, 40);
    lv_obj_align(btnResetB, LV_ALIGN_BOTTOM_RIGHT, -20, -16);
    lv_obj_add_event_cb(btnResetB, onResetTripBClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblResetB = lv_label_create(btnResetB);
    lv_label_set_text(lblResetB, "Reset B");
    lv_obj_center(lblResetB);
}

void DisplayManager::buildNotificationsScreen() {
    _screenNotifications = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenNotifications, lv_color_hex(0x0A0E14), 0);
    attachGestureNav(_screenNotifications);

    lv_obj_t* title = lv_label_create(_screenNotifications);
    lv_label_set_text(title, "Notifications");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    _notifList = lv_list_create(_screenNotifications);
    lv_obj_set_size(_notifList, SCREEN_W - 40, SCREEN_H - 90);
    lv_obj_align(_notifList, LV_ALIGN_TOP_MID, 0, 40);

    _labelNoNotifications = lv_label_create(_screenNotifications);
    lv_label_set_text(_labelNoNotifications, "No active notifications");
    lv_obj_set_style_text_color(_labelNoNotifications, lv_color_hex(0x8A93A8), 0);
    lv_obj_center(_labelNoNotifications);
}

void DisplayManager::buildSettingsScreen() {
    _screenSettings = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screenSettings, lv_color_hex(0x0A0E14), 0);
    attachGestureNav(_screenSettings);

    lv_obj_t* title = lv_label_create(_screenSettings);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    // --- Theme cycle button ---
    lv_obj_t* btnTheme = lv_btn_create(_screenSettings);
    lv_obj_set_size(btnTheme, 200, 40);
    lv_obj_align(btnTheme, LV_ALIGN_TOP_LEFT, 20, 50);
    lv_obj_add_event_cb(btnTheme, onThemeButtonClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblThemeBtn = lv_label_create(btnTheme);
    lv_label_set_text(lblThemeBtn, "Theme >");
    lv_obj_center(lblThemeBtn);

    _labelThemeValue = lv_label_create(_screenSettings);
    lv_obj_set_style_text_color(_labelThemeValue, lv_color_white(), 0);
    lv_obj_align_to(_labelThemeValue, btnTheme, LV_ALIGN_OUT_RIGHT_MID, 12, 0);
    lv_label_set_text(_labelThemeValue, "Modern Digital");

    // --- Ride mode cycle button ---
    lv_obj_t* btnRideMode = lv_btn_create(_screenSettings);
    lv_obj_set_size(btnRideMode, 200, 40);
    lv_obj_align(btnRideMode, LV_ALIGN_TOP_LEFT, 20, 100);
    lv_obj_add_event_cb(btnRideMode, onRideModeButtonClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblRideModeBtn = lv_label_create(btnRideMode);
    lv_label_set_text(lblRideModeBtn, "Ride Mode >");
    lv_obj_center(lblRideModeBtn);

    _labelRideModeValue = lv_label_create(_screenSettings);
    lv_obj_set_style_text_color(_labelRideModeValue, lv_color_white(), 0);
    lv_obj_align_to(_labelRideModeValue, btnRideMode, LV_ALIGN_OUT_RIGHT_MID, 12, 0);
    lv_label_set_text(_labelRideModeValue, "City");

    // --- Brightness slider ---
    lv_obj_t* lblBrightness = lv_label_create(_screenSettings);
    lv_label_set_text(lblBrightness, "Brightness");
    lv_obj_set_style_text_color(lblBrightness, lv_color_white(), 0);
    lv_obj_align(lblBrightness, LV_ALIGN_TOP_LEFT, 20, 150);

    _sliderBrightness = lv_slider_create(_screenSettings);
    lv_obj_set_size(_sliderBrightness, 240, 20);
    lv_obj_align(_sliderBrightness, LV_ALIGN_TOP_LEFT, 20, 180);
    lv_slider_set_range(_sliderBrightness, 20, 255);   // floor at 20 — never fully black, still readable
    lv_slider_set_value(_sliderBrightness, 200, LV_ANIM_OFF);
    lv_obj_add_event_cb(_sliderBrightness, [](lv_event_t* e) {
        lv_obj_t* slider = lv_event_get_target(e);
        ledcWrite(0, lv_slider_get_value(slider));
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    // --- Maintenance (right column) — closes the loop on SERVICE_DUE /
    // CHAIN_LUBE_DUE warnings actually meaning something. Tyre/insurance/
    // PUC aren't editable here (no on-screen date/number entry widget yet
    // — see docs/maintenance.md) and are configured via BLE from the
    // companion app instead. ---
    lv_obj_t* lblMaintTitle = lv_label_create(_screenSettings);
    lv_label_set_text(lblMaintTitle, "Maintenance");
    lv_obj_set_style_text_color(lblMaintTitle, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(lblMaintTitle, LV_ALIGN_TOP_LEFT, 260, 50);

    _labelServiceStatus = lv_label_create(_screenSettings);
    lv_obj_set_style_text_color(_labelServiceStatus, lv_color_white(), 0);
    lv_obj_align(_labelServiceStatus, LV_ALIGN_TOP_LEFT, 260, 76);
    lv_label_set_text(_labelServiceStatus, "Service: --");

    lv_obj_t* btnMarkServiced = lv_btn_create(_screenSettings);
    lv_obj_set_size(btnMarkServiced, 190, 36);
    lv_obj_align(btnMarkServiced, LV_ALIGN_TOP_LEFT, 260, 100);
    lv_obj_add_event_cb(btnMarkServiced, onMarkServicedClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblMarkServiced = lv_label_create(btnMarkServiced);
    lv_label_set_text(lblMarkServiced, "Mark Serviced");
    lv_obj_center(lblMarkServiced);

    _labelChainStatus = lv_label_create(_screenSettings);
    lv_obj_set_style_text_color(_labelChainStatus, lv_color_white(), 0);
    lv_obj_align(_labelChainStatus, LV_ALIGN_TOP_LEFT, 260, 150);
    lv_label_set_text(_labelChainStatus, "Chain: --");

    lv_obj_t* btnMarkChain = lv_btn_create(_screenSettings);
    lv_obj_set_size(btnMarkChain, 190, 36);
    lv_obj_align(btnMarkChain, LV_ALIGN_TOP_LEFT, 260, 174);
    lv_obj_add_event_cb(btnMarkChain, onMarkChainLubedClicked, LV_EVENT_CLICKED, this);
    lv_obj_t* lblMarkChain = lv_label_create(btnMarkChain);
    lv_label_set_text(lblMarkChain, "Mark Chain Lubed");
    lv_obj_center(lblMarkChain);

    // --- Hardware status readout (diagnostic, read-only) ---
    _labelSdStatus = lv_label_create(_screenSettings);
    lv_obj_set_style_text_color(_labelSdStatus, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelSdStatus, LV_ALIGN_BOTTOM_LEFT, 20, -60);
    lv_label_set_text(_labelSdStatus, "SD: --");

    _labelGpsStatus = lv_label_create(_screenSettings);
    lv_obj_set_style_text_color(_labelGpsStatus, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelGpsStatus, LV_ALIGN_BOTTOM_LEFT, 20, -36);
    lv_label_set_text(_labelGpsStatus, "GPS: --");

    _labelBleStatus = lv_label_create(_screenSettings);
    lv_obj_set_style_text_color(_labelBleStatus, lv_color_hex(0x8A93A8), 0);
    lv_obj_align(_labelBleStatus, LV_ALIGN_BOTTOM_LEFT, 20, -12);
    lv_label_set_text(_labelBleStatus, "BLE: --");
}

void DisplayManager::applyTheme(ThemeMode mode) {
    // Full theme system swaps color tokens / fonts per docs/themes.md — kept
    // to a single accent-color swap here to illustrate the hook point without
    // duplicating ten near-identical theme tables in sample code.
    lv_color_t accent = lv_color_hex(0x00D4FF);
    switch (mode) {
        case ThemeMode::SPORT:  accent = lv_color_hex(0xFF1744); break;
        case ThemeMode::NEON:   accent = lv_color_hex(0x39FF14); break;
        case ThemeMode::RETRO:  accent = lv_color_hex(0xFF8A00); break;
        default: break;
    }
    lv_obj_set_style_arc_color(_arcRpm, accent, LV_PART_INDICATOR);
}

void DisplayManager::goToScreen(Screen s) {
    // Direction-aware animation: moving "forward" in the cycle order
    // (Main -> Trip -> Notifications -> Settings -> Main) slides left,
    // backward slides right — matches whichever swipe direction got you
    // there, per docs/screen_flow.md's spatial-consistency rule. The forced
    // critical-notification jump (see tick()) still goes through this same
    // path rather than a distinct fade — docs/screen_flow.md's "fade
    // instead" note is aspirational polish for a future pass, not yet
    // implemented; functionally it still reaches Notifications immediately
    // either way, which is what the safety rule actually requires.
    lv_obj_t* target = nullptr;
    switch (s) {
        case Screen::MAIN_DASHBOARD:  target = _screenMain; break;
        case Screen::TRIP_INFO:        target = _screenTripInfo; break;
        case Screen::NOTIFICATIONS:     target = _screenNotifications; refreshNotificationsList(); break;
        case Screen::SETTINGS:           target = _screenSettings; refreshSettingsLabels(); break;
    }
    if (!target) return;

    bool forward = (uint8_t)s > (uint8_t)_currentScreen ||
                     (_currentScreen == Screen::SETTINGS && s == Screen::MAIN_DASHBOARD);
    _currentScreen = s;

    // First-ever screen load (during begin()) has no previous screen to
    // slide away from — load it directly rather than animating from nothing.
    if (lv_disp_get_scr_act(nullptr) == nullptr) {
        lv_scr_load(target);
        return;
    }
    lv_scr_load_anim(target, forward ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

// ------------------------------------------------------- Per-frame refresh
void DisplayManager::refreshWidgetsFromState() {
    VehicleState s = SharedState::instance().snapshot();

    lv_label_set_text_fmt(_labelSpeed, "%d", (int)s.speedKmh);
    lv_arc_set_value(_arcRpm, s.rpm);

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

    lv_label_set_text_fmt(_labelTrip, "A %.1f km  |  ODO %.0f km", s.tripA_km, s.odometer_km);
    lv_bar_set_value(_barFuel, (int)s.fuelLevelPct, LV_ANIM_ON);
    lv_label_set_text_fmt(_labelEngineTemp, "%.0fC", s.engineTempC);

    s.inLeftIndicator  ? lv_obj_clear_flag(_iconLeftIndicator, LV_OBJ_FLAG_HIDDEN)  : lv_obj_add_flag(_iconLeftIndicator, LV_OBJ_FLAG_HIDDEN);
    s.inRightIndicator ? lv_obj_clear_flag(_iconRightIndicator, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(_iconRightIndicator, LV_OBJ_FLAG_HIDDEN);
    s.inNeutral          ? lv_obj_clear_flag(_iconNeutral, LV_OBJ_FLAG_HIDDEN)          : lv_obj_add_flag(_iconNeutral, LV_OBJ_FLAG_HIDDEN);
    s.inHighBeam          ? lv_obj_clear_flag(_iconHighBeam, LV_OBJ_FLAG_HIDDEN)          : lv_obj_add_flag(_iconHighBeam, LV_OBJ_FLAG_HIDDEN);

    // Warning banner reflects NotificationManager's top unacknowledged item
    // — suppressed while riding above QUIET_MODE_SPEED_THRESHOLD_KMH unless
    // it's CRITICAL (DisplayPolicyMath::shouldSuppressBanner guarantees
    // CRITICAL is never suppressed, so this can never hide something that
    // actually matters — it's purely about not popping up "Chain Lube Due"
    // while you're doing 60 on the highway). The notification itself is
    // never removed from the queue by this — it's still sitting there and
    // still shows on the Notifications screen; this only hides the
    // Main-Dashboard banner.
    const Notification* n = NotificationManager::instance().current();
    bool suppress = n && DisplayPolicyMath::shouldSuppressBanner(
        n->priority == NotifPriority::CRITICAL, s.speedKmh, QUIET_MODE_SPEED_THRESHOLD_KMH);
    if (n && !suppress) {
        lv_label_set_text(_labelWarningBanner, n->title.c_str());
        lv_obj_clear_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_labelWarningBanner, LV_OBJ_FLAG_HIDDEN);
    }

    // Clock — cheap to update every frame since it's just a label; real
    // build should throttle this to 1x/sec via a static last-second check.
    static int lastSec = -1;
    time_t now = time(nullptr);
    struct tm* tmv = localtime(&now);
    if (tmv->tm_sec != lastSec) {
        lastSec = tmv->tm_sec;
        lv_label_set_text_fmt(_labelClock, "%02d:%02d", tmv->tm_hour, tmv->tm_min);
    }

    // --- Trip Info screen (only worth updating when it's actually visible,
    // but the label writes are cheap enough that gating on _currentScreen
    // isn't necessary — LVGL skips rendering hidden screens anyway) ---
    lv_label_set_text_fmt(_labelTripA, "Trip A: %.1f km", s.tripA_km);
    lv_label_set_text_fmt(_labelTripB, "Trip B: %.1f km", s.tripB_km);
    lv_label_set_text_fmt(_labelOdometer, "Odometer: %.0f km", s.odometer_km);
    uint32_t rt = s.rideTimerSec;
    lv_label_set_text_fmt(_labelRideTimer, "Ride Time: %02u:%02u", rt / 3600, (rt / 60) % 60);
    lv_label_set_text_fmt(_labelAvgSpeed, "Avg Speed: %.0f km/h", s.avgSpeedKmh);
    lv_label_set_text_fmt(_labelMaxSpeed, "Max Speed: %.0f km/h", s.maxSpeedKmh);
    if (s.fuelRangeKm > 0) lv_label_set_text_fmt(_labelFuelRange, "Range: %.0f km", s.fuelRangeKm);
    if (s.fuelConsumptionKmL > 0) lv_label_set_text_fmt(_labelFuelConsumption, "Efficiency: %.1f km/L", s.fuelConsumptionKmL);

    // --- Settings screen status readout ---
    lv_label_set_text_fmt(_labelSdStatus, "SD: %s", s.sdCardOk ? "OK" : "Not detected");
    lv_label_set_text_fmt(_labelGpsStatus, "GPS: %s", s.gpsFixValid ? "Fix" : (s.gpsModuleOk ? "Searching" : "Not detected"));
    lv_label_set_text_fmt(_labelBleStatus, "BLE: %s", s.bleConnected ? "Connected" : "Waiting");

    // Service/chain status — reads directly from MaintenanceManager rather
    // than duplicating the due-km values in VehicleState, since these
    // change rarely (only on "mark done") and don't need to ride through
    // the shared-state snapshot every frame like live sensor data does.
    auto& maint = MaintenanceManager::instance();
    float remainingSvc = maint.serviceDueKm() - s.odometer_km;
    if (remainingSvc >= 0) lv_label_set_text_fmt(_labelServiceStatus, "Service: %.0f km left", remainingSvc);
    else lv_label_set_text_fmt(_labelServiceStatus, "Service: OVERDUE by %.0f km", -remainingSvc);

    float remainingChain = maint.chainDueKm() - s.odometer_km;
    if (remainingChain >= 0) lv_label_set_text_fmt(_labelChainStatus, "Chain: %.0f km left", remainingChain);
    else lv_label_set_text_fmt(_labelChainStatus, "Chain: OVERDUE by %.0f km", -remainingChain);
}

void DisplayManager::refreshNotificationsList() {
    if (!_notifList) return;
    lv_obj_clean(_notifList);   // rebuild — the list is small (MAX_QUEUE=16), cheap to redo

    auto& nm = NotificationManager::instance();
    size_t shown = 0;
    for (size_t i = 0; i < nm.count(); i++) {
        const Notification& n = nm.at(i);
        if (n.acknowledged) continue;
        const char* icon = n.priority == NotifPriority::CRITICAL ? LV_SYMBOL_WARNING
                          : n.priority == NotifPriority::WARNING ? LV_SYMBOL_BELL
                                                                    : LV_SYMBOL_LIST;
        lv_obj_t* btn = lv_list_add_btn(_notifList, icon, n.title.c_str());
        // Stash the notification's queue index in the button so the click
        // handler knows which one to acknowledge — see onNotificationRowClicked.
        lv_obj_set_user_data(btn, (void*)(uintptr_t)i);
        lv_obj_add_event_cb(btn, onNotificationRowClicked, LV_EVENT_CLICKED, this);
        shown++;
    }

    if (shown == 0) lv_obj_clear_flag(_labelNoNotifications, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(_labelNoNotifications, LV_OBJ_FLAG_HIDDEN);
}

void DisplayManager::refreshSettingsLabels() {
    const char* themeNames[] = { "Light", "Dark", "Classic Analog", "Modern Digital", "Minimal",
                                  "Sport", "Retro", "Neon", "Cyberpunk", "Custom" };
    lv_label_set_text(_labelThemeValue, themeNames[(uint8_t)_selectedTheme]);

    const char* rideModeNames[] = { "Eco", "City", "Touring", "Sport", "Rain", "Custom" };
    lv_label_set_text(_labelRideModeValue, rideModeNames[(uint8_t)_selectedRideMode]);
}

void DisplayManager::refreshPhoneLinkWidgets() {
    PhoneLinkState link = PhoneLinkManager::instance().snapshot();
    uint32_t now = millis();

    // --- Incoming call modal ---
    bool callFresh = DisplayPolicyMath::isDataFresh(link.callUpdatedMs, now, CALL_STALE_MS);
    bool shouldShowCall = link.callActive && callFresh;
    if (shouldShowCall && !_callModalShown) {
        lv_label_set_text(_labelCallerName, link.callerName.c_str());
        lv_obj_clear_flag(_callModal, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(_callModal);
        _callModalShown = true;
    } else if (!shouldShowCall && _callModalShown) {
        lv_obj_add_flag(_callModal, LV_OBJ_FLAG_HIDDEN);
        _callModalShown = false;
    }

    // --- Music widget (Main Dashboard only) ---
    if (DisplayPolicyMath::isDataFresh(link.musicUpdatedMs, now, PHONE_LINK_STALE_MS)) {
        char trackText[96];
        if (link.musicArtist.length() > 0) {
            snprintf(trackText, sizeof(trackText), "%s - %s", link.musicTitle.c_str(), link.musicArtist.c_str());
        } else {
            snprintf(trackText, sizeof(trackText), "%s", link.musicTitle.c_str());
        }
        lv_label_set_text(_labelMusicTrack, trackText);
        lv_obj_clear_flag(_musicWidget, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_musicWidget, LV_OBJ_FLAG_HIDDEN);
    }

    // --- Navigation banner (Main Dashboard only) ---
    if (DisplayPolicyMath::isDataFresh(link.navUpdatedMs, now, PHONE_LINK_STALE_MS)) {
        lv_label_set_text_fmt(_labelNavInstruction, "%s - %.0fm", link.navInstruction.c_str(), link.navDistanceM);
        lv_obj_clear_flag(_navBanner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_navBanner, LV_OBJ_FLAG_HIDDEN);
    }
}

// --------------------------------------------------------- Button callbacks
void DisplayManager::applyRideModeProfile(RideMode mode, bool persist) {
    const RideModeProfile& profile = RideModeProfiles::get(mode);
    bool enteringRain = persist && (mode == RideMode::RAIN) && (_selectedRideMode != RideMode::RAIN);

    _selectedRideMode = mode;
    SharedState::instance().update([&](VehicleState& s) { s.rideMode = mode; });

    if (persist) {
        // A real mode change (not a boot-time restore) applies the mode's
        // default theme, per the spec's "each mode modifies display theme."
        // A manual Theme-button pick made earlier is intentionally
        // overwritten here — see docs/themes.md for the documented
        // interaction: mode change resets theme, manual cycling afterward
        // is a temporary override good until the next mode change.
        _selectedTheme = profile.theme;
        applyTheme(_selectedTheme);
        StorageManager::instance().saveTheme((uint8_t)_selectedTheme);
        StorageManager::instance().saveRideMode((uint8_t)mode);
    }

    // Brightness cap always applies, restore or real change — clamp
    // whatever the slider currently shows down to the new mode's ceiling
    // rather than forcing a specific value, so a user's finer-grained
    // choice within that ceiling (e.g. 180 out of a 220 Touring cap)
    // survives a mode switch that doesn't lower the cap below it.
    lv_slider_set_range(_sliderBrightness, 20, profile.maxBrightness);
    uint8_t current = (uint8_t)lv_slider_get_value(_sliderBrightness);
    uint8_t clamped = current < profile.maxBrightness ? current : profile.maxBrightness;
    lv_slider_set_value(_sliderBrightness, clamped, LV_ANIM_ON);
    ledcWrite(0, clamped);

    if (enteringRain) {
        NotificationManager::instance().push(
            "Rain Mode", "Reduced traction advisory - ride cautiously", NotifPriority::INFO);
    }
}

void DisplayManager::onThemeButtonClicked(lv_event_t* e) {
    DisplayManager* self = (DisplayManager*)lv_event_get_user_data(e);
    uint8_t next = ((uint8_t)self->_selectedTheme + 1) % ((uint8_t)ThemeMode::CUSTOM + 1);
    self->_selectedTheme = (ThemeMode)next;
    self->applyTheme(self->_selectedTheme);
    StorageManager::instance().saveTheme((uint8_t)self->_selectedTheme);
    self->refreshSettingsLabels();
    // Manual theme choice persists on its own — see applyRideModeProfile's
    // comment for how this interacts with the next ride-mode change.
}

void DisplayManager::onRideModeButtonClicked(lv_event_t* e) {
    DisplayManager* self = (DisplayManager*)lv_event_get_user_data(e);
    uint8_t next = ((uint8_t)self->_selectedRideMode + 1) % ((uint8_t)RideMode::CUSTOM + 1);
    self->applyRideModeProfile((RideMode)next, /*persist=*/true);
    self->refreshSettingsLabels();
}

void DisplayManager::onResetTripAClicked(lv_event_t* e) {
    (void)e;
    RideManager::instance().resetTripA();
}

void DisplayManager::onResetTripBClicked(lv_event_t* e) {
    (void)e;
    RideManager::instance().resetTripB();
}

void DisplayManager::onMarkServicedClicked(lv_event_t* e) {
    (void)e;
    MaintenanceManager::instance().markServiced();
}

void DisplayManager::onMarkChainLubedClicked(lv_event_t* e) {
    (void)e;
    MaintenanceManager::instance().markChainLubed();
}

void DisplayManager::onCallAcceptClicked(lv_event_t* e) {
    DisplayManager* self = (DisplayManager*)lv_event_get_user_data(e);
    PhoneLinkManager::instance().requestCallAccept();
    lv_obj_add_flag(self->_callModal, LV_OBJ_FLAG_HIDDEN);
    self->_callModalShown = false;
    // Doesn't call PhoneLinkManager::endCall() here — the phone is the
    // source of truth for whether the call actually ended; this only
    // dismisses the LOCAL modal so it's not still covering the screen
    // while the actual call happens on the phone (typically over a
    // Bluetooth headset). The phone app should follow up with
    // {"cmd":"call_ended"} once the call genuinely ends.
}

void DisplayManager::onCallRejectClicked(lv_event_t* e) {
    DisplayManager* self = (DisplayManager*)lv_event_get_user_data(e);
    PhoneLinkManager::instance().requestCallReject();
    lv_obj_add_flag(self->_callModal, LV_OBJ_FLAG_HIDDEN);
    self->_callModalShown = false;
    PhoneLinkManager::instance().endCall();   // rejecting is locally final — no need to wait for the phone to confirm
}

void DisplayManager::onMusicPlayPauseClicked(lv_event_t* e) {
    (void)e;
    PhoneLinkManager::instance().requestMusicPlayPause();
}

void DisplayManager::onMusicNextClicked(lv_event_t* e) {
    (void)e;
    PhoneLinkManager::instance().requestMusicNext();
}

void DisplayManager::onNotificationRowClicked(lv_event_t* e) {
    DisplayManager* self = (DisplayManager*)lv_event_get_user_data(e);
    lv_obj_t* btn = lv_event_get_target(e);
    size_t index = (size_t)(uintptr_t)lv_obj_get_user_data(btn);
    NotificationManager::instance().acknowledgeAt(index);
    self->refreshNotificationsList();
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
