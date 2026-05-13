#pragma once
#include "../config.h"

class StatusBar {
public:
    static StatusBar& instance();
    void draw();   // renders to canvas, marks dirty
    void tick();   // call every second to update time

    void setBattery(uint8_t pct) { _battery = pct; }
    void setLoRa(bool active)    { _loraActive = active; }
    void setGPS(bool fix)        { _gpsFix = fix; }
    void setWiFi(bool conn)      { _wifi = conn; }

private:
    StatusBar() = default;
    uint8_t _battery    = 100;
    bool    _loraActive = false;
    bool    _gpsFix     = false;
    bool    _wifi       = false;
    char    _timeBuf[8] = "00:00";
};
