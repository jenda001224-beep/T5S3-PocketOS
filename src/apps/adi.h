#pragma once
#include "app.h"
#include <Arduino.h>

#define ADI_APP_ID  11

// Advanced Device Info — a live, scrollable dump of everything the device
// currently knows about itself: SoC, memory, power/battery, radios, storage,
// clock and sensors. Refreshes once per second while open.
class AdiApp : public App {
public:
    AdiApp() : App("ADI", ADI_APP_ID) {}
    void onEnter() override;
    void draw()    override;
    void tick()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    static constexpr int MAX_LINES = 120;
    String  _lines[MAX_LINES];
    bool    _isHeader[MAX_LINES];
    int     _n      = 0;
    int     _scroll = 0;

    void build();
    void add(const char* k, const String& v);
    void section(const char* title);
};
