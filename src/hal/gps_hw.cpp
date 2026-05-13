#include "gps_hw.h"

GPSHW& GPSHW::instance() {
    static GPSHW inst;
    return inst;
}

bool GPSHW::begin() {
    Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
    _ready = true;
    return true;
}

void GPSHW::update() {
    if (!_ready) return;
    while (Serial2.available()) _gps.encode(Serial2.read());
}

GPSFix GPSHW::fix() const {
    GPSFix f;
    f.valid  = _gps.location.isValid();
    f.lat    = _gps.location.lat();
    f.lng    = _gps.location.lng();
    f.alt    = _gps.altitude.meters();
    f.speed  = _gps.speed.kmph();
    f.course = _gps.course.deg();
    f.sats   = _gps.satellites.value();
    return f;
}

bool GPSHW::hasFix() const {
    return _gps.location.isValid() && _gps.location.age() < 3000;
}
