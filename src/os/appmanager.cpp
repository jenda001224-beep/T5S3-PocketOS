#include "appmanager.h"
#include "../hal/touch.h"
#include "../hal/gps_hw.h"
#include "../hal/lora_hw.h"
#include "../hal/epd.h"
#include "../ui/canvas.h"
#include "../ui/homescreen.h"
#include "../ui/controlcenter.h"
#include "../ui/statusbar.h"
#include "../apps/settings.h"

AppManager& AppManager::instance() {
    static AppManager inst;
    return inst;
}

void AppManager::registerApp(App* app) {
    if (_appCount < MAX_APPS) _apps[_appCount++] = app;
}

App* findApp(App** apps, int count, uint8_t id) {
    for (int i = 0; i < count; i++) if (apps[i]->id() == id) return apps[i];
    return nullptr;
}

void AppManager::goHome() {
    if (_current) {
        _current->onLeave();
        _current = nullptr;
    }
    HomeScreen::instance().draw();
}

void AppManager::launchApp(uint8_t id) {
    App* app = findApp(_apps, _appCount, id);
    if (!app) return;
    if (_current) _current->onLeave();
    _current = app;
    _current->onEnter();
    _lastActivity = millis();
}

void AppManager::begin() {
    // Home screen is always shown at start
    goHome();
    _lastActivity = millis();
}

void AppManager::loop() {
    uint32_t now = millis();

    // GPS feed
    GPSHW::instance().update();

    // Status bar update
    StatusBar& sb = StatusBar::instance();
    sb.setLoRa(LoRaHW::instance().ready());
    sb.setGPS(GPSHW::instance().hasFix());

    // 1-second tick
    if (now - _lastTick >= 1000) {
        _lastTick = now;
        StatusBar::instance().tick();
        if (_current) _current->tick();
    }

    // Auto sleep
    uint16_t sleepSec = SettingsApp::config().sleepSec;
    if (SettingsApp::config().deepSleepAuto && sleepSec > 0 &&
        now - _lastActivity > (uint32_t)sleepSec * 1000) {
        EPD::instance().setFrontlight(0);
        esp_deep_sleep_start();
    }

    // Touch
    TouchEvent ev = Touch::instance().poll();
    if (ev.type == TouchEvent::NONE) return;

    _lastActivity = now;

    // Control center: swipe down from top 60px of screen
    ControlCenter& cc = ControlCenter::instance();
    if (!cc.isOpen() && ev.type == TouchEvent::SWIPE_DOWN && ev.y < 60) {
        cc.open();
        return;
    }
    if (cc.isOpen()) {
        cc.handleEvent(ev);
        return;
    }

    if (_current) {
        _current->handleEvent(ev);
    } else {
        // Home screen
        int launch = HomeScreen::instance().handleEvent(ev);
        if (launch >= 0) launchApp((uint8_t)launch);
    }
}
