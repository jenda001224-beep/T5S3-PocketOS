#include "power_hw.h"
#include <Wire.h>

Power& Power::instance() {
    static Power inst;
    return inst;
}

bool Power::reg8Read(uint8_t addr, uint8_t reg, uint8_t& val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) return false;
    val = Wire.read();
    return true;
}

bool Power::reg8Write(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

bool Power::burstRead(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

void Power::begin() {
    // Detect + configure BQ25896: enable continuous ADC (REG02 bit6 CONV_RATE=1).
    uint8_t r2;
    if (reg8Read(BQ25896_ADDR, 0x02, r2)) {
        _bq = true;
        reg8Write(BQ25896_ADDR, 0x02, r2 | 0x40);   // CONV_RATE = continuous
    }
    // Detect PCF8563 (a read of the control register).
    uint8_t c;
    _rtc = reg8Read(RTC_ADDR, 0x00, c);
}

BatteryInfo Power::battery() {
    BatteryInfo b;
    if (!_bq) return b;

    uint8_t v;
    if (reg8Read(BQ25896_ADDR, 0x0E, v)) b.millivolts = 2304 + (v & 0x7F) * 20;
    if (reg8Read(BQ25896_ADDR, 0x0F, v)) b.sysMv      = 2304 + (v & 0x7F) * 20;
    if (reg8Read(BQ25896_ADDR, 0x11, v)) b.vbusMv     = (v & 0x7F) ? 2600 + (v & 0x7F) * 100 : 0;
    if (reg8Read(BQ25896_ADDR, 0x12, v)) b.chargeMa   = (v & 0x7F) * 50;

    uint8_t s;
    if (reg8Read(BQ25896_ADDR, 0x0B, s)) {
        b.chargeStat = (s >> 3) & 0x03;
        b.charging   = (b.chargeStat == 1 || b.chargeStat == 2);
        b.usb        = ((s >> 2) & 0x01) || (((s >> 5) & 0x07) != 0);
    }

    int pct = ((int)b.millivolts - BATTERY_MIN_MV) * 100 / (BATTERY_MAX_MV - BATTERY_MIN_MV);
    b.percent = (uint8_t)constrain(pct, 0, 100);
    return b;
}

static uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

bool Power::getTime(struct tm& out) {
    if (!_rtc) return false;
    uint8_t r[7];
    if (!burstRead(RTC_ADDR, 0x02, r, 7)) return false;
    if (r[0] & 0x80) return false;   // VL flag: clock integrity lost
    out.tm_sec  = bcd2dec(r[0] & 0x7F);
    out.tm_min  = bcd2dec(r[1] & 0x7F);
    out.tm_hour = bcd2dec(r[2] & 0x3F);
    out.tm_mday = bcd2dec(r[3] & 0x3F);
    out.tm_wday = r[4] & 0x07;
    out.tm_mon  = bcd2dec(r[5] & 0x1F) - 1;
    out.tm_year = bcd2dec(r[6]) + ((r[5] & 0x80) ? 200 : 100);
    out.tm_isdst = 0;
    return true;
}

bool Power::setTime(const struct tm& in) {
    if (!_rtc) return false;
    int year = in.tm_year + 1900;
    uint8_t century = (year >= 2000) ? 0x80 : 0x00;
    Wire.beginTransmission(RTC_ADDR);
    Wire.write(0x02);
    Wire.write(dec2bcd(in.tm_sec)  & 0x7F);
    Wire.write(dec2bcd(in.tm_min));
    Wire.write(dec2bcd(in.tm_hour));
    Wire.write(dec2bcd(in.tm_mday));
    Wire.write(in.tm_wday & 0x07);
    Wire.write(dec2bcd(in.tm_mon + 1) | century);
    Wire.write(dec2bcd((uint8_t)(year % 100)));
    return Wire.endTransmission() == 0;
}
