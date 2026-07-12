#include "settings.h"
#include "../ui/canvas.h"
#include "../hal/epd.h"
#include "../hal/storage.h"
#include "../hal/lora_hw.h"
#include "../os/appmanager.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ctype.h>
#include <math.h>

static SystemConfig _cfg;

SystemConfig& SettingsApp::config() { return _cfg; }
void SettingsApp::saveConfigStatic() { SettingsApp tmp; tmp.saveConfig(); }
void SettingsApp::loadConfigStatic() { SettingsApp tmp; tmp.loadConfig(); }

const char* SettingsApp::KBD_ROWS[] = {
    "1234567890",
    "qwertyuiop",
    "asdfghjkl",
    "zxcvbnm@._",
    "^ <#",          // shift · space · backspace · OK(#)
};

const char* SettingsApp::TZ_PRESETS[] = {
    "CET-1CEST,M3.5.0,M10.5.0/3",   // Central Europe (Prague)
    "GMT0BST,M3.5.0/1,M10.5.0",     // UK
    "EST5EDT,M3.2.0,M11.1.0",       // US East
    "PST8PDT,M3.2.0,M11.1.0",       // US West
    "UTC0",                          // UTC
};
const int SettingsApp::TZ_COUNT = sizeof(TZ_PRESETS) / sizeof(TZ_PRESETS[0]);

static const char* tzShort(const char* tz) {
    if (strncmp(tz, "CET", 3) == 0) return "Europe (CET)";
    if (strncmp(tz, "GMT", 3) == 0) return "UK (GMT)";
    if (strncmp(tz, "EST", 3) == 0) return "US East";
    if (strncmp(tz, "PST", 3) == 0) return "US West";
    return "UTC";
}

void SettingsApp::loadConfig() {
    String json = Storage::instance().readText(CONFIG_PATH);
    if (json.isEmpty()) {
        _cfg.brightness    = 80;
        _cfg.fontScale     = 2;
        _cfg.loraFreq      = LORA_FREQ;
        _cfg.gpsEnabled    = true;
        _cfg.wifiEnabled   = false;
        _cfg.deepSleepAuto = false;   // off by default (deep sleep has no touch wake yet)
        _cfg.sleepSec      = 300;
        _cfg.invertColors  = false;
        strncpy(_cfg.deviceName, "PocketOS", 31);
        strncpy(_cfg.timezone,   TZ_PRESETS[0], 39);
        _cfg.wifiSSID[0] = 0;
        _cfg.wifiPass[0] = 0;
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, json);
    _cfg.brightness    = doc["brightness"]   | 80;
    _cfg.fontScale     = doc["fontScale"]    | 2;
    _cfg.loraFreq      = doc["loraFreq"]     | LORA_FREQ;
    _cfg.gpsEnabled    = doc["gpsEnabled"]   | true;
    _cfg.wifiEnabled   = doc["wifiEnabled"]  | false;
    _cfg.deepSleepAuto = doc["autoSleep"]    | false;
    _cfg.sleepSec      = doc["sleepSec"]     | 300;
    _cfg.invertColors  = doc["invertColors"] | false;
    strncpy(_cfg.deviceName, doc["deviceName"] | "PocketOS", 31);
    strncpy(_cfg.timezone,   doc["timezone"]   | TZ_PRESETS[0], 39);
    strncpy(_cfg.wifiSSID,   doc["wifiSSID"]   | "", 32);
    strncpy(_cfg.wifiPass,   doc["wifiPass"]   | "", 64);
}

void SettingsApp::saveConfig() {
    JsonDocument doc;
    doc["brightness"]   = _cfg.brightness;
    doc["fontScale"]    = _cfg.fontScale;
    doc["loraFreq"]     = _cfg.loraFreq;
    doc["gpsEnabled"]   = _cfg.gpsEnabled;
    doc["wifiEnabled"]  = _cfg.wifiEnabled;
    doc["autoSleep"]    = _cfg.deepSleepAuto;
    doc["sleepSec"]     = _cfg.sleepSec;
    doc["invertColors"] = _cfg.invertColors;
    doc["deviceName"]   = _cfg.deviceName;
    doc["timezone"]     = _cfg.timezone;
    doc["wifiSSID"]     = _cfg.wifiSSID;
    doc["wifiPass"]     = _cfg.wifiPass;
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
        cv.drawText(EPD_W - 260, y + 14, value, C_MID, odd ? C_WHITE : 12, 2);
    }
    if (arrow) cv.drawText(EPD_W - 20, y + 18, ">", C_MID, odd ? C_WHITE : 12, 1);
}

void SettingsApp::drawMainMenu() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Settings");

    int y = STATUS_BAR_H + 4;
    drawRow(y, "Display",     "", true);  y += 44;
    drawRow(y, "Wi-Fi",       _cfg.wifiEnabled ? _cfg.wifiSSID : "Off", true);  y += 44;
    drawRow(y, "LoRa Radio",  "", true);  y += 44;
    drawRow(y, "System",      "", true);  y += 44;

    cv.drawHLine(0, y, EPD_W, C_LIGHT);
    y += 8;
    cv.drawText(30, y, "PocketOS v1.1  ·  T5 E-Paper S3 Pro", C_MID, C_WHITE, 1); y += 16;

    uint64_t total = Storage::instance().totalBytes();
    uint64_t used  = Storage::instance().usedBytes();
    char buf[56];
    snprintf(buf, sizeof(buf), "SD: %llu MB / %llu MB used", used>>20, total>>20);
    cv.drawText(30, y, buf, C_MID, C_WHITE, 1); y += 16;
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
    cv.fillRoundRect(40, y, EPD_W-80, 16, 8, C_LIGHT);
    int fill = (int)(((float)_cfg.brightness / 255) * (EPD_W - 84));
    cv.fillRoundRect(40, y, fill, 16, 8, C_MID);
    y += 32;

    snprintf(val, sizeof(val), "%dx", _cfg.fontScale);
    drawRow(y, "Font Scale", val, false); y += 44;
    for (int i = 1; i <= 4; i++) {
        int bx = 40 + (i-1) * 80;
        cv.fillRoundRect(bx, y, 70, 32, 6, _cfg.fontScale == i ? C_MID : C_LIGHT);
        char fs[4]; snprintf(fs, sizeof(fs), "%dx", i);
        cv.drawTextCentered(bx+35, y+10, 70, fs, C_BLACK, _cfg.fontScale == i ? C_MID : C_LIGHT, 2);
    }
    y += 48;

    drawRow(y, "Invert Colors", _cfg.invertColors ? "ON" : "OFF", false);
    y += 44;
    cv.drawText(30, y + 6, "(brightness stored; e-paper has no backlight)", C_MID, C_WHITE, 1);

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

    float freqs[] = {433.0f, 868.0f, 915.0f};
    const char* names[] = {"433", "868", "915"};
    for (int i = 0; i < 3; i++) {
        int bx = 40 + i * 120;
        bool sel = fabsf(_cfg.loraFreq - freqs[i]) < 1.0f;
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
    char val[40];

    drawRow(y, "Device Name", _cfg.deviceName, false); y += 44;
    drawRow(y, "GPS",         _cfg.gpsEnabled ? "ON" : "OFF", false); y += 44;
    snprintf(val, sizeof(val), "%ds", _cfg.sleepSec);
    drawRow(y, "Auto Sleep",  _cfg.deepSleepAuto ? val : "OFF", false); y += 44;
    drawRow(y, "Timezone",    tzShort(_cfg.timezone), false); y += 44;

    cv.fillRoundRect(30, y, 160, 36, 8, C_MID);
    cv.drawTextCentered(110, y+12, 160, "Reboot", C_BLACK, C_MID, 2);
    cv.fillRoundRect(220, y, 200, 36, 8, C_DARK);
    cv.drawTextCentered(320, y+12, 200, "Factory Reset", C_WHITE, C_DARK, 1);

    cv.flushFull();
}

void SettingsApp::drawKeyboard(int y0) {
    Canvas& cv = Canvas::instance();
    int kbdH = EPD_H - y0;
    cv.fillRect(0, y0, EPD_W, kbdH, C_LIGHT);
    int nrows = 5;
    int rowH  = kbdH / nrows;
    for (int r = 0; r < nrows; r++) {
        const char* keys = KBD_ROWS[r];
        int nkeys = strlen(keys);
        int keyW  = EPD_W / nkeys;
        for (int k = 0; k < nkeys; k++) {
            int kx = k * keyW, ky = y0 + r * rowH;
            cv.fillRoundRect(kx+2, ky+2, keyW-4, rowH-4, 4, C_WHITE);
            char c = keys[k];
            const char* lbl;
            char tmp[6] = {0};
            if (c == '^') lbl = _shift ? "ABC" : "abc";
            else if (c == '<') lbl = "DEL";
            else if (c == '#') lbl = "OK";
            else if (c == ' ') lbl = "space";
            else { tmp[0] = _shift ? toupper(c) : c; lbl = tmp; }
            cv.drawTextCentered(kx+keyW/2, ky+rowH/2-8, keyW, lbl, C_BLACK, C_WHITE, 2);
        }
    }
}

void SettingsApp::handleKeyboard(const TouchEvent& ev, int y0) {
    int kbdH  = EPD_H - y0;
    int nrows = 5;
    int rowH  = kbdH / nrows;
    int row   = (ev.y - y0) / rowH;
    if (row < 0 || row >= nrows) return;
    const char* keys = KBD_ROWS[row];
    int nkeys = strlen(keys);
    int keyW  = EPD_W / nkeys;
    int col   = ev.x / keyW;
    if (col < 0 || col >= nkeys) return;
    char c = keys[col];
    if (c == '^')      { _shift = !_shift; }
    else if (c == '<') { if (_pwdLen > 0) _pwd[--_pwdLen] = 0; }
    else if (c == '#') { connectSelected(); return; }
    else if (_pwdLen < 63) {
        _pwd[_pwdLen++] = _shift ? toupper(c) : c;
        _pwd[_pwdLen] = 0;
    }
    draw();
}

void SettingsApp::startScan() {
    _scanning = true;
    draw();               // show "Scanning…"
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();
    _scanCount = 0;
    for (int i = 0; i < n && _scanCount < 24; i++) {
        _scan[_scanCount++] = WiFi.SSID(i);
    }
    _scanning = false;
    draw();
}

void SettingsApp::connectSelected() {
    if (_selNet < 0 || _selNet >= _scanCount) return;
    strncpy(_cfg.wifiSSID, _scan[_selNet].c_str(), 32);
    strncpy(_cfg.wifiPass, _pwd, 64);
    _cfg.wifiEnabled = true;
    saveConfig();
    WiFi.begin(_cfg.wifiSSID, _cfg.wifiPass);
    _wifiState = WIFI_LIST;
    draw();
}

void SettingsApp::drawWiFiSettings() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Wi-Fi");

    if (_wifiState == WIFI_PASS) {
        int y = STATUS_BAR_H + 6;
        char line[80];
        snprintf(line, sizeof(line), "Network: %s",
            (_selNet >= 0 && _selNet < _scanCount) ? _scan[_selNet].c_str() : "?");
        cv.drawText(20, y, line, C_BLACK, C_WHITE, 2); y += 30;
        cv.drawText(20, y, "Password:", C_MID, C_WHITE, 1); y += 18;
        cv.fillRoundRect(20, y, EPD_W-40, 30, 6, C_LIGHT);
        char shown[80]; snprintf(shown, sizeof(shown), "%s_", _pwd);
        cv.drawText(28, y+8, shown, C_BLACK, C_LIGHT, 2);
        drawKeyboard(EPD_H/2 - 10);
        cv.flushFull();
        return;
    }

    int y = STATUS_BAR_H + 4;
    drawRow(y, "Wi-Fi", _cfg.wifiEnabled ? "ON" : "OFF", false); y += 44;

    // status line
    char st[80];
    if (WiFi.status() == WL_CONNECTED)
        snprintf(st, sizeof(st), "Connected: %s  %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    else if (_cfg.wifiSSID[0])
        snprintf(st, sizeof(st), "Saved: %s (not connected)", _cfg.wifiSSID);
    else
        snprintf(st, sizeof(st), "No network saved");
    cv.drawText(30, y+6, st, C_MID, C_WHITE, 1); y += 30;

    // Scan button
    cv.fillRoundRect(30, y, 160, 34, 8, C_MID);
    cv.drawTextCentered(110, y+10, 160, _scanning ? "Scanning…" : "Scan", C_BLACK, C_MID, 2);
    y += 46;

    // Network list
    int lineH = 34;
    int maxVis = (EPD_H - y - DOCK_H) / lineH;
    for (int i = 0; i < _scanCount && i < maxVis; i++) {
        bool odd = (i % 2) == 0;
        uint8_t bg = odd ? C_WHITE : 12;
        cv.fillRect(0, y, EPD_W, lineH-2, bg);
        cv.drawText(30, y+8, _scan[i].c_str(), C_BLACK, bg, 2);
        cv.drawText(EPD_W-24, y+12, ">", C_MID, bg, 1);
        y += lineH;
    }
    cv.flushFull();
}

void SettingsApp::draw() {
    switch (_page) {
        case PAGE_MAIN:    drawMainMenu();        break;
        case PAGE_DISPLAY: drawDisplaySettings(); break;
        case PAGE_LORA:    drawLoRaSettings();    break;
        case PAGE_SYSTEM:  drawSystemSettings();  break;
        case PAGE_WIFI:    drawWiFiSettings();    break;
    }
}

void SettingsApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) {
        if (_page == PAGE_WIFI && _wifiState == WIFI_PASS) { _wifiState = WIFI_LIST; draw(); }
        else if (_page != PAGE_MAIN) { _page = PAGE_MAIN; draw(); }
        else AppManager::instance().goHome();
        return;
    }

    if (ev.type != TouchEvent::TAP) return;

    if (_page == PAGE_MAIN) {
        int row = (ev.y - STATUS_BAR_H) / 44;
        switch (row) {
            case 0: _page = PAGE_DISPLAY; break;
            case 1: _page = PAGE_WIFI; _wifiState = WIFI_LIST; break;
            case 2: _page = PAGE_LORA;    break;
            case 3: _page = PAGE_SYSTEM;  break;
            default: return;
        }
        draw();
        return;
    }

    if (_page == PAGE_DISPLAY) {
        int sliderY = STATUS_BAR_H + 48;
        if (ev.y >= sliderY && ev.y < sliderY + 16) {
            int b = (int)(((float)(ev.x - 40) / (EPD_W-80)) * 255);
            _cfg.brightness = constrain(b, 0, 255);
            EPD::instance().setFrontlight(_cfg.brightness);
            saveConfig(); draw(); return;
        }
        int fsY = STATUS_BAR_H + 92;
        if (ev.y >= fsY && ev.y < fsY + 32) {
            int i = (ev.x - 40) / 80 + 1;
            if (i >= 1 && i <= 4) { _cfg.fontScale = i; saveConfig(); draw(); }
            return;
        }
        if ((ev.y - STATUS_BAR_H) / 44 == 3) {
            _cfg.invertColors = !_cfg.invertColors; saveConfig(); draw();
        }
        return;
    }

    if (_page == PAGE_LORA) {
        float freqs[] = {433.0f, 868.0f, 915.0f};
        int btnY = STATUS_BAR_H + 44*5 + 4;
        if (ev.y >= btnY && ev.y < btnY + 32) {
            int i = (ev.x - 40) / 120;
            if (i >= 0 && i < 3) {
                _cfg.loraFreq = freqs[i];
                LoRaHW::instance().setFrequency(_cfg.loraFreq);
                saveConfig(); draw();
            }
        }
        return;
    }

    if (_page == PAGE_SYSTEM) {
        int row = (ev.y - STATUS_BAR_H - 4) / 44;
        if (row == 1) { _cfg.gpsEnabled = !_cfg.gpsEnabled; saveConfig(); draw(); return; }
        if (row == 2) {
            if (!_cfg.deepSleepAuto) { _cfg.deepSleepAuto = true; _cfg.sleepSec = 120; }
            else if (_cfg.sleepSec == 120) _cfg.sleepSec = 300;
            else if (_cfg.sleepSec == 300) _cfg.sleepSec = 600;
            else _cfg.deepSleepAuto = false;
            saveConfig(); draw(); return;
        }
        if (row == 3) {
            // cycle timezone preset
            int cur = 0;
            for (int i = 0; i < TZ_COUNT; i++) if (strcmp(_cfg.timezone, TZ_PRESETS[i]) == 0) cur = i;
            cur = (cur + 1) % TZ_COUNT;
            strncpy(_cfg.timezone, TZ_PRESETS[cur], 39);
            setenv("TZ", _cfg.timezone, 1); tzset();
            saveConfig(); draw(); return;
        }
        int btnY = STATUS_BAR_H + 44*4 + 4;
        if (ev.y >= btnY && ev.y < btnY + 36) {
            if (ev.x < 200) { saveConfig(); ESP.restart(); }
            else {
                Storage::instance().writeText(CONFIG_PATH, "{}");
                ESP.restart();
            }
        }
        return;
    }

    if (_page == PAGE_WIFI) {
        if (_wifiState == WIFI_PASS) { handleKeyboard(ev, EPD_H/2 - 10); return; }

        int y = STATUS_BAR_H + 4;
        // Wi-Fi on/off row
        if (ev.y >= y && ev.y < y + 44) {
            _cfg.wifiEnabled = !_cfg.wifiEnabled;
            if (!_cfg.wifiEnabled) WiFi.disconnect(true);
            else if (_cfg.wifiSSID[0]) WiFi.begin(_cfg.wifiSSID, _cfg.wifiPass);
            saveConfig(); draw(); return;
        }
        // Scan button
        int scanY = STATUS_BAR_H + 4 + 44 + 30;
        if (ev.y >= scanY && ev.y < scanY + 34 && ev.x < 200) { startScan(); return; }
        // Network list
        int listY = scanY + 46;
        int lineH = 34;
        if (ev.y >= listY) {
            int idx = (ev.y - listY) / lineH;
            if (idx >= 0 && idx < _scanCount) {
                _selNet = idx;
                _pwd[0] = 0; _pwdLen = 0; _shift = false;
                _wifiState = WIFI_PASS;
                draw();
            }
        }
        return;
    }
}
