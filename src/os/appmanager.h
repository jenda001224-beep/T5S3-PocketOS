#pragma once
#include "../apps/app.h"
#include "../ui/homescreen.h"

class AppManager {
public:
    static AppManager& instance();

    void    begin();
    void    loop();           // call every iteration from main loop()
    void    goHome();
    void    launchApp(uint8_t id);
    App*    currentApp() const { return _current; }
    bool    isHome() const     { return _current == nullptr; }

    void    registerApp(App* app);

private:
    AppManager() = default;

    static constexpr int MAX_APPS = 16;
    App*    _apps[MAX_APPS] = {};
    int     _appCount       = 0;
    App*    _current        = nullptr;

    uint32_t _lastTick      = 0;
    uint32_t _lastActivity  = 0;
    bool     _ccTriggered   = false;
};
