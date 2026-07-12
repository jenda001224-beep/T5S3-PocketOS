#pragma once
#include <Arduino.h>
#include <time.h>
#include "../config.h"

// Battery / charger telemetry read from the BQ25896 over the shared I2C bus,
// plus the PCF8563 real-time clock. Both live on SDA39/SCL40.
struct BatteryInfo {
    uint16_t millivolts = 0;   // VBAT
    uint8_t  percent     = 0;   // 0-100 (li-ion curve)
    bool     charging    = false;
    bool     usb         = false; // VBUS / power good
    uint16_t sysMv       = 0;   // SYSV
    uint16_t vbusMv      = 0;   // VBUS voltage
    int16_t  chargeMa    = 0;   // charge current
    uint8_t  chargeStat  = 0;   // 0=idle 1=pre 2=fast 3=done
};

class Power {
public:
    static Power& instance();

    void begin();                 // configure BQ25896 ADC + detect RTC
    BatteryInfo battery();

    bool rtcPresent() const { return _rtc; }
    bool bqPresent()  const { return _bq; }

    bool getTime(struct tm& out);           // from PCF8563
    bool setTime(const struct tm& in);      // set PCF8563

private:
    Power() = default;
    bool    _rtc = false;
    bool    _bq  = false;

    bool    reg8Read(uint8_t addr, uint8_t reg, uint8_t& val);
    bool    reg8Write(uint8_t addr, uint8_t reg, uint8_t val);
    bool    burstRead(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t len);
};
