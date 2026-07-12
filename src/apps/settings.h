#pragma once
#include "app.h"

#define SETTINGS_APP_ID  6

struct SystemConfig {
    uint8_t  brightness;    // 0-255 (stored; no physical frontlight on this board)
    uint8_t  fontScale;     // 1-4
    float    loraFreq;      // MHz
    char     deviceName[32];
    bool     gpsEnabled;
    bool     wifiEnabled;
    bool     deepSleepAuto; // sleep after idle
    uint16_t sleepSec;
    bool     invertColors;
    char     timezone[40];  // POSIX TZ string
    char     wifiSSID[33];
    char     wifiPass[65];
};

class SettingsApp : public App {
public:
    SettingsApp() : App("Settings", SETTINGS_APP_ID) {}

    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

    static SystemConfig& config();
    static void saveConfigStatic();   // used by other apps (e.g. Control Center)
    static void loadConfigStatic();   // called once at boot (main.cpp)

private:
    enum Page { PAGE_MAIN, PAGE_DISPLAY, PAGE_LORA, PAGE_SYSTEM, PAGE_WIFI };
    enum WifiState { WIFI_LIST, WIFI_PASS };

    void drawMainMenu();
    void drawDisplaySettings();
    void drawLoRaSettings();
    void drawSystemSettings();
    void drawWiFiSettings();
    void drawKeyboard(int y0);
    void handleKeyboard(const TouchEvent& ev, int y0);
    void startScan();
    void connectSelected();
    void saveConfig();
    void loadConfig();

    Page  _page   = PAGE_MAIN;
    int   _scroll = 0;

    // WiFi sub-view
    WifiState _wifiState = WIFI_LIST;
    String    _scan[24];
    int       _scanCount = 0;
    int       _selNet    = -1;
    char      _pwd[65]   = "";
    int       _pwdLen    = 0;
    bool      _scanning  = false;
    bool      _shift     = false;

    static const char* KBD_ROWS[];
    static const char* TZ_PRESETS[];
    static const int   TZ_COUNT;
};
