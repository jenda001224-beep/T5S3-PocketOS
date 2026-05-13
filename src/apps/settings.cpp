#include "settings.h"
#include "../ui/canvas.h"
#include "../hal/epd.h"
#include "../hal/storage.h"
#include "../os/appmanager.h"
#include <ArduinoJson.h>
#include <WiFi.h>

static SystemConfig _cfg;

SystemConfig& SettingsApp::config() { return _cfg; }

void SettingsApp::loadConfig() {
    String json = Storage::instance().readText(CONFIG_PATH);
    if (json.isEmpty()) {
        // Defaults
        _cfg.brightness   = 80;
        _cfg.fontScale    = 2;
        _cfg.loraFreq     = LORA_FREQ;
        _cfg.gpsEnabled   = true;
        _cfg.deepSleepAuto = true;
        _cfg.sleepSec     = 300;
        _cfg.invertColors = false;
        strncpy(_cfg.deviceName, "PocketOS", 31);
        strncpy(_cfg.timezone,   "CET-1CEST,M3.5.0,M10.5.0/3", 31);
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, json);
    _cfg.brightness    = doc["brightness"]   | 80;
    _cfg.fontScale     = doc["fontScale"]    | 2;
    _cfg.loraFreq      = doc["loraFreq"]     | LORA_FREQ;
    _cfg.gpsEnabled    = doc["gpsEnabled"]   | true;
    _cfg.deepSleepAuto = doc["autoSleep"]    | true;
    _cfg.sleepSec      = doc["sleepSec"]     | 300;
    _cfg.invertColors  = doc["invertColors"] | false;
    strncpy(_cfg.deviceName, doc["deviceName"] | "PocketOS", 31);
    strncpy(_cfg.timezone,   doc["timezone"]   | "CET-1CEST,M3.5.0,M10.5.0/3", 31);
}

void SettingsApp::saveConfig() {
    JsonDocument doc;
    doc["brightness"]   = _cfg.brightness;
    doc["fontScale"]    = _cfg.fontScale;
    doc["loraFreq"]     = _cfg.loraFreq;
    doc["gpsEnabled"]   = _cfg.gpsEnabled;
    doc["autoSleep"]    = _cfg.deepSleepAuto;
    doc["sleepSec"]     = _cfg.sleepSec;
    doc["invertColors"] = _cfg.invertColors;
    doc["deviceName"]   = _cfg.deviceName;
    doc["timezone"]     = _cfg.timezone;
    String out;
    serializeJson(doc, out);
    Storage::instance().writeText(CONFIG_PATH, out);
}

void SettingsApp::onEnter() {
    loadConfig();
    _page   = PAGE_MAIN;
    _scroll = 0;
    draw();
}

static void drawRow(int y, const char* label, const char* value, bool arrow=true) {
    Canvas& cv = Canvas::instance();
    bool odd = (y / 44) % 2 == 0;
    cv.fillRect(0, y, EPD_W, 44, odd ? C_WHITE : 12);
    cv.drawText(30, y + 14, label, C_BLACK, odd ? C_WHITE : 12, 2);
    if (value && value[0]) {
        cv.drawText(EPD_W - 160, y + 14, value, C_MID, odd ? C_WHITE : 12, 2);
    }
    if (arrow) cv.drawText(EPD_W - 20, y + 18, ">", C_MID, odd ? C_WHITE : 12, 1);
}

void SettingsApp::drawMainMenu() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Settings");

    int y = STATUS_BAR_H + 4;
    drawRow(y, "Display",     "", true);  y += 44;
    drawRow(y, "LoRa Radio",  "", true);  y += 44;
    drawRow(y, "System",      "", true);  y += 44;
    drawRow(y, "About",       "", true);  y += 44;

    // About section as flat info
    cv.drawHLine(0, y, EPD_W, C_LIGHT);
    y += 8;
    cv.drawText(30, y,    "PocketOS v1.0", C_MID, C_WHITE, 1); y += 16;
    cv.drawText(30, y,    "LILYGO T5S3 4.7\" E-Paper", C_MID, C_WHITE, 1); y += 16;

    // Storage info
    uint64_t total = Storage::instance().totalBytes();
    uint64_t used  = Storage::instance().usedBytes();
    char buf[48];
    snprintf(buf, sizeof(buf), "SD: %llu MB / %llu MB used", used>>20, total>>20);
    cv.drawText(30, y, buf, C_MID, C_WHITE, 1); y += 16;

    // Chip info
    snprintf(buf, sizeof(buf), "ESP32-S3  %dMHz  %dMB PSRAM",
        ESP.getCpuFreqMHz(), ESP.getPsramSize()>>20);
    cv.drawText(30, y, buf, C_MID, C_WHITE, 1);

    cv.flushFull();
}

void SettingsApp::drawDisplaySettings() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Display");

    int y = STATUS_BAR_H + 4;
    char val[16];

    snprintf(val, sizeof(val), "%d%%", (int)(_cfg.brightness/2.55f));
    drawRow(y, "Brightness", val, false); y += 44;
    // Brightness slider
    cv.fillRoundRect(40, y, EPD_W-80, 16, 8, C_LIGHT);
    int fill = (int)(((float)_cfg.brightness / 255) * (EPD_W - 84));
    cv.fillRoundRect(40, y, fill, 16, 8, C_MID);
    y += 32;

    snprintf(val, sizeof(val), "%dx", _cfg.fontScale);
    drawRow(y, "Font Scale", val, false); y += 44;
    // Font scale buttons
    for (int i = 1; i <= 4; i++) {
        int bx = 40 + (i-1) * 80;
        cv.fillRoundRect(bx, y, 70, 32, 6, _cfg.fontScale == i ? C_MID : C_LIGHT);
        char fs[4]; snprintf(fs, sizeof(fs), "%dx", i);
        cv.drawTextCentered(bx+35, y+10, 70, fs, C_BLACK, _cfg.fontScale == i ? C_MID : C_LIGHT, 2);
    }
    y += 48;

    drawRow(y, "Invert Colors", _cfg.invertColors ? "ON" : "OFF", false);
    y += 44;

    cv.flushFull();
}

void SettingsApp::drawLoRaSettings() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("LoRa Radio");

    int y = STATUS_BAR_H + 4;
    char val[32];

    snprintf(val, sizeof(val), "%.1f MHz", _cfg.loraFreq);
    drawRow(y, "Frequency", val, false); y += 44;

    drawRow(y, "Bandwidth",   "125 kHz",     false); y += 44;
    drawRow(y, "Spread. Factor", "SF10",     false); y += 44;
    drawRow(y, "TX Power",    "17 dBm",      false); y += 44;
    drawRow(y, "Sync Word",   "0x34",        false); y += 44;

    // Freq buttons
    float freqs[] = {433.0f, 868.0f, 915.0f};
    const char* names[] = {"433", "868", "915"};
    for (int i = 0; i < 3; i++) {
        int bx = 40 + i * 120;
        bool sel = abs(_cfg.loraFreq - freqs[i]) < 1.0f;
        cv.fillRoundRect(bx, y, 100, 32, 6, sel ? C_MID : C_LIGHT);
        cv.drawTextCentered(bx+50, y+10, 100, names[i], C_BLACK, sel ? C_MID : C_LIGHT, 2);
    }

    cv.flushFull();
}

void SettingsApp::drawSystemSettings() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("System");

    int y = STATUS_BAR_H + 4;
    char val[32];

    drawRow(y, "Device Name", _cfg.deviceName, false); y += 44;
    drawRow(y, "GPS",         _cfg.gpsEnabled ? "ON" : "OFF", false); y += 44;
    snprintf(val, sizeof(val), "%ds", _cfg.sleepSec);
    drawRow(y, "Auto Sleep",  _cfg.deepSleepAuto ? val : "OFF", false); y += 44;
    drawRow(y, "Timezone",    _cfg.timezone, false); y += 44;

    // Reboot + factory reset buttons
    cv.fillRoundRect(30, y, 160, 36, 8, C_MID);
    cv.drawTextCentered(110, y+12, 160, "Reboot", C_BLACK, C_MID, 2);

    cv.fillRoundRect(220, y, 200, 36, 8, C_DARK);
    cv.drawTextCentered(320, y+12, 200, "Factory Reset", C_WHITE, C_DARK, 1);

    cv.flushFull();
}

void SettingsApp::draw() {
    switch (_page) {
        case PAGE_MAIN:    drawMainMenu();       break;
        case PAGE_DISPLAY: drawDisplaySettings(); break;
        case PAGE_LORA:    drawLoRaSettings();    break;
        case PAGE_SYSTEM:  drawSystemSettings();  break;
    }
}

void SettingsApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) {
        if (_page != PAGE_MAIN) { _page = PAGE_MAIN; draw(); }
        else AppManager::instance().goHome();
        return;
    }

    if (ev.type == TouchEvent::TAP) {
        if (_page == PAGE_MAIN) {
            int row = (ev.y - STATUS_BAR_H) / 44;
            switch (row) {
                case 0: _page = PAGE_DISPLAY; break;
                case 1: _page = PAGE_LORA;    break;
                case 2: _page = PAGE_SYSTEM;  break;
            }
            draw();
            return;
        }
        if (_page == PAGE_DISPLAY) {
            // Brightness slider
            int sliderY = STATUS_BAR_H + 48;
            if (ev.y >= sliderY && ev.y < sliderY + 16) {
                _cfg.brightness = (uint8_t)(((float)(ev.x - 40) / (EPD_W-80)) * 255);
                _cfg.brightness = constrain(_cfg.brightness, 0, 255);
                EPD::instance().setFrontlight(_cfg.brightness);
                saveConfig(); draw(); return;
            }
            // Font scale buttons
            int fsY = STATUS_BAR_H + 44 + 44 + 4;
            if (ev.y >= fsY && ev.y < fsY + 32) {
                int i = (ev.x - 40) / 80 + 1;
                if (i >= 1 && i <= 4) { _cfg.fontScale = i; saveConfig(); draw(); }
                return;
            }
            // Invert
            if ((ev.y - STATUS_BAR_H) / 44 == 3) {
                _cfg.invertColors = !_cfg.invertColors;
                saveConfig(); draw();
            }
        }
        if (_page == PAGE_LORA) {
            float freqs[] = {433.0f, 868.0f, 915.0f};
            int btnY = STATUS_BAR_H + 44*5 + 4;
            if (ev.y >= btnY && ev.y < btnY + 32) {
                int i = (ev.x - 40) / 120;
                if (i >= 0 && i < 3) { _cfg.loraFreq = freqs[i]; saveConfig(); draw(); }
            }
        }
        if (_page == PAGE_SYSTEM) {
            int btnY = STATUS_BAR_H + 44*4 + 4;
            if (ev.y >= btnY && ev.y < btnY + 36) {
                if (ev.x < 220) { saveConfig(); ESP.restart(); }
            }
        }
    }
}
