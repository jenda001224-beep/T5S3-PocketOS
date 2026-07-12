#include "controlcenter.h"
#include "canvas.h"
#include "../hal/epd.h"
#include "../hal/lora_hw.h"
#include "../apps/settings.h"
#include <WiFi.h>

ControlCenter& ControlCenter::instance() {
    static ControlCenter inst;
    return inst;
}

void ControlCenter::open() {
    // Reflect the live system state whenever the panel is pulled down.
    SystemConfig& c = SettingsApp::config();
    _brightness = c.brightness;
    _fontSize   = c.fontScale;
    _wifiOn     = c.wifiEnabled;
    _gpsOn      = c.gpsEnabled;
    _loraOn     = LoRaHW::instance().enabled();
    _open = true;
    draw();
}

void ControlCenter::close() {
    _open = false;
}

void ControlCenter::drawSlider(const Slider& s) {
    Canvas& cv = Canvas::instance();
    // Label
    cv.drawText(s.x, s.y - 18, s.label, C_WHITE, C_BLACK, 1);
    // Track
    cv.fillRoundRect(s.x, s.y, s.w, 20, 10, C_DARK);
    // Fill
    int fill = (int)(((float)(*s.value - s.min) / (s.max - s.min)) * (s.w - 4));
    cv.fillRoundRect(s.x + 2, s.y + 2, max(16, fill), 16, 8, C_WHITE);
    // Knob
    int kx = s.x + 2 + max(8, fill) - 10;
    cv.fillCircle(kx + 10, s.y + 10, 12, C_WHITE);
    cv.drawCircle(kx + 10, s.y + 10, 12, C_MID);
}

void ControlCenter::drawToggle(int x, int y, const char* label, bool on) {
    Canvas& cv = Canvas::instance();
    cv.drawText(x, y - 18, label, C_WHITE, C_BLACK, 1);
    cv.fillRoundRect(x, y, 60, 30, 15, on ? C_LIGHT : C_DARK);
    int kx = on ? x + 32 : x + 2;
    cv.fillCircle(kx + 13, y + 15, 11, C_WHITE);
}

void ControlCenter::draw() {
    if (!_open) return;
    Canvas& cv = Canvas::instance();

    // Frosted panel background
    cv.fillRoundRect(0, 0, PANEL_W, PANEL_H, 0, C_BLACK);
    cv.fillRoundRect(4, 4, PANEL_W - 8, PANEL_H - 8, 16, C_DARK);

    // Title
    cv.drawTextCentered(PANEL_W / 2, 14, 300, "Control Center", C_WHITE, C_DARK, 2);

    // Pull handle
    cv.fillRoundRect(PANEL_W / 2 - 30, PANEL_H - 20, 60, 6, 3, C_MID);

    int y = 60;

    // Brightness slider
    Slider brightSlider{ "Brightness", PAD, y, PANEL_W - 2*PAD, &_brightness, 0, 255 };
    drawSlider(brightSlider);
    y += 60;

    // Font size slider
    Slider fontSlider{ "Font Size", PAD, y, PANEL_W - 2*PAD, &_fontSize, 1, 4 };
    drawSlider(fontSlider);
    y += 70;

    // Toggles row
    drawToggle(PAD,               y, "LoRa",  _loraOn);
    drawToggle(PAD + 120,         y, "WiFi",  _wifiOn);
    drawToggle(PAD + 240,         y, "GPS",   _gpsOn);
    y += 60;

    // Sleep button
    cv.fillRoundRect(PANEL_W / 2 - 60, y, 120, 36, 8, C_MID);
    cv.drawTextCentered(PANEL_W / 2, y + 12, 120, "Sleep", C_BLACK, C_MID, 2);

    cv.flushDirty();  // flushRegion not available; use flushDirty for partial update
}

bool ControlCenter::handleEvent(const TouchEvent& ev) {
    if (!_open) return false;

    if (ev.type == TouchEvent::SWIPE_UP) {
        close();
        return true;
    }

    if (ev.type == TouchEvent::TAP || ev.type == TouchEvent::MOVE) {
        int tx = ev.x, ty = ev.y;

        // Outside panel — close
        if (ty > PANEL_H) { close(); return true; }

        SystemConfig& c = SettingsApp::config();

        // Brightness slider
        if (ty >= 60 && ty <= 80 && tx >= PAD && tx < EPD_W - PAD) {
            _brightness = (uint8_t)(((float)(tx - PAD) / (EPD_W - 2*PAD)) * 255);
            EPD::instance().setFrontlight(_brightness);
            c.brightness = _brightness;
            SettingsApp::saveConfigStatic();
            draw();
            return true;
        }
        // Font size slider
        if (ty >= 120 && ty <= 140 && tx >= PAD && tx < EPD_W - PAD) {
            _fontSize = 1 + (uint8_t)(((float)(tx - PAD) / (EPD_W - 2*PAD)) * 3);
            c.fontScale = _fontSize;
            SettingsApp::saveConfigStatic();
            draw();
            return true;
        }

        int toggleY = 190;
        // LoRa toggle
        if (ty >= toggleY && ty < toggleY + 30 && tx >= PAD && tx < PAD + 60) {
            _loraOn = !_loraOn;
            LoRaHW::instance().setEnabled(_loraOn);
            draw();
            return true;
        }
        // WiFi toggle
        if (ty >= toggleY && ty < toggleY + 30 && tx >= PAD+120 && tx < PAD+180) {
            _wifiOn = !_wifiOn;
            c.wifiEnabled = _wifiOn;
            if (_wifiOn) { if (c.wifiSSID[0]) WiFi.begin(c.wifiSSID, c.wifiPass); }
            else WiFi.disconnect(true);
            SettingsApp::saveConfigStatic();
            draw();
            return true;
        }
        // GPS toggle
        if (ty >= toggleY && ty < toggleY + 30 && tx >= PAD+240 && tx < PAD+300) {
            _gpsOn = !_gpsOn;
            c.gpsEnabled = _gpsOn;
            SettingsApp::saveConfigStatic();
            draw();
            return true;
        }
        // Sleep button
        if (ty >= 250 && tx >= EPD_W/2 - 60 && tx < EPD_W/2 + 60) {
            esp_deep_sleep_start();
            return true;
        }
    }
    return true;  // consume all events when open
}
