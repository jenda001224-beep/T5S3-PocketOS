#pragma once
#include "app.h"

#define SETTINGS_APP_ID  6

struct SystemConfig {
    uint8_t brightness;    // 0-255
    uint8_t fontScale;     // 1-4
    float   loraFreq;      // MHz
    char    deviceName[32];
    bool    gpsEnabled;
    bool    wifiEnabled;
    bool    deepSleepAuto; // sleep after idle
    uint16_t sleepSec;
    bool    invertColors;
    char    timezone[32];  // e.g. "CET-1CEST"
};

class SettingsApp : public App {
public:
    SettingsApp() : App("Settings", SETTINGS_APP_ID) {}

    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

    static SystemConfig& config();

private:
    enum Page { PAGE_MAIN, PAGE_DISPLAY, PAGE_LORA, PAGE_SYSTEM };
    void drawMainMenu();
    void drawDisplaySettings();
    void drawLoRaSettings();
    void drawSystemSettings();
    void saveConfig();
    void loadConfig();

    Page  _page   = PAGE_MAIN;
    int   _scroll = 0;

    struct MenuItem { const char* label; const char* value; Page subPage; };
};
