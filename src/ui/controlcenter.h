#pragma once
#include "../config.h"
#include "../hal/touch.h"

// Slides in from top on SWIPE_DOWN from status bar area.
// Contains: brightness, font size, LoRa toggle, WiFi toggle, sleep.
class ControlCenter {
public:
    static ControlCenter& instance();

    bool   isOpen()  const { return _open; }
    void   open();
    void   close();
    void   draw();
    bool   handleEvent(const TouchEvent& ev);  // returns true if consumed

    uint8_t brightness()  const { return _brightness; }
    uint8_t fontSize()    const { return _fontSize; }
    bool    loraEnabled() const { return _loraOn; }
    bool    wifiEnabled() const { return _wifiOn; }

private:
    ControlCenter() = default;

    struct Slider {
        const char* label;
        int x, y, w;
        uint8_t* value;
        uint8_t  min, max;
    };

    void drawSlider(const Slider& s);
    void drawToggle(int x, int y, const char* label, bool on);
    int  sliderHit(const Slider& s, int tx, int ty);

    bool    _open       = false;
    uint8_t _brightness = 80;
    uint8_t _fontSize   = 2;    // 1-3
    bool    _loraOn     = true;
    bool    _wifiOn     = false;
    bool    _gpsOn      = true;

    static constexpr int PANEL_H = 280;
    static constexpr int PANEL_W = EPD_W;
    static constexpr int PAD     = 30;
};
