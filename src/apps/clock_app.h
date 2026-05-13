#pragma once
#include "app.h"

#define CLOCK_APP_ID  7

class ClockApp : public App {
public:
    ClockApp() : App("Clock", CLOCK_APP_ID) {}

    void onEnter() override;
    void draw()    override;
    void tick()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    // Renamed to avoid collision with Arduino's #define ANALOG 0xC0
    enum View { VIEW_ANALOG, VIEW_DIGITAL, VIEW_CALENDAR };
    void drawAnalog();
    void drawDigital();
    void drawCalendar();

    View _view = VIEW_DIGITAL;
};
