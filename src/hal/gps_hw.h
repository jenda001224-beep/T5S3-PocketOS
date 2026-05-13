#pragma once
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "../config.h"

struct GPSFix {
    double  lat, lng;
    float   alt;       // metres
    float   speed;     // km/h
    float   course;    // degrees
    uint8_t sats;
    bool    valid;
};

class GPSHW {
public:
    static GPSHW& instance();
    bool     begin();
    void     update();           // feed serial bytes into TinyGPS++
    GPSFix   fix() const;
    bool     hasFix() const;

private:
    GPSHW() = default;
    mutable TinyGPSPlus _gps;  // mutable: lat()/lng() etc. are non-const in TinyGPS++
    bool        _ready = false;
};
