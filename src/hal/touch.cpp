#include "touch.h"

Touch& Touch::instance() {
    static Touch inst;
    return inst;
}

bool Touch::readGT911(uint8_t* buf, uint16_t reg, uint8_t len) {
    Wire.beginTransmission(_addr);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    if (Wire.endTransmission() != 0) return false;
    Wire.requestFrom(_addr, len);
    for (int i = 0; i < len && Wire.available(); i++) buf[i] = Wire.read();
    return true;
}

void Touch::writeGT911(uint16_t reg, uint8_t val) {
    Wire.beginTransmission(_addr);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    Wire.write(val);
    Wire.endTransmission();
}

bool Touch::probe(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

// GT911 latches its I2C address from the INT level during the RST rising edge:
// INT high -> 0x14, INT low -> 0x5D.
void Touch::hwReset(bool pickPrimaryAddr) {
    pinMode(TOUCH_RST, OUTPUT);
    pinMode(TOUCH_INT, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    digitalWrite(TOUCH_INT, pickPrimaryAddr ? HIGH : LOW);
    delay(12);
    digitalWrite(TOUCH_RST, HIGH);
    delay(12);
    pinMode(TOUCH_INT, INPUT);   // release INT for interrupt/idle use
    delay(60);
}

bool Touch::begin() {
    // FastEPD already brought up Wire on the shared bus; re-assert pins anyway.
    Wire.begin(TOUCH_SDA, TOUCH_SCL, 400000);

    // Reset selecting the primary address, then fall back to the alternate.
    hwReset(true);
    _addr = 0x14;
    if (!probe(_addr)) {
        hwReset(false);
        _addr = 0x5D;
        if (!probe(_addr)) {
            // last resort: probe both without a reset
            _addr = 0x14; if (!probe(_addr)) { _addr = 0x5D; if (!probe(_addr)) return false; }
        }
    }
    Serial.printf("[TOUCH] GT911 @ 0x%02X\n", _addr);

    // Issue soft reset and configure 960×540 resolution
    writeGT911(0x8040, 0x02);  // soft reset
    delay(10);
    writeGT911(0x8040, 0x00);

    // Set output resolution registers
    writeGT911(0x8048, EPD_W & 0xFF);
    writeGT911(0x8049, EPD_W >> 8);
    writeGT911(0x804A, EPD_H & 0xFF);
    writeGT911(0x804B, EPD_H >> 8);

    // Refresh config checksum
    uint8_t cfg[1];
    readGT911(cfg, 0x8047, 1);
    writeGT911(0x80FF, cfg[0]);
    writeGT911(0x8100, 0x01);  // config fresh

    return true;
}

TouchEvent Touch::poll() {
    TouchEvent ev;
    ev.type = TouchEvent::NONE;

    // Read status register
    uint8_t status;
    if (!readGT911(&status, 0x814E, 1)) return ev;

    uint8_t count = status & 0x0F;
    bool    valid = (status & 0x80) != 0;

    // Clear status
    writeGT911(0x814E, 0x00);

    TouchPoint cur;
    cur.pressed = valid && count > 0;

    if (cur.pressed) {
        uint8_t raw[8];
        readGT911(raw, 0x8150, 8);
        cur.x = ((int16_t)raw[1] << 8) | raw[0];
        cur.y = ((int16_t)raw[3] << 8) | raw[2];
        // Clamp
        cur.x = constrain(cur.x, 0, EPD_W - 1);
        cur.y = constrain(cur.y, 0, EPD_H - 1);
    }

    if (cur.pressed && !_wasDown) {
        // Finger down
        _downX    = cur.x;
        _downY    = cur.y;
        _downTime = millis();
        _longFired = false;
        ev.type = TouchEvent::DOWN;
        ev.x = cur.x; ev.y = cur.y;
    } else if (cur.pressed && _wasDown) {
        // Long press check
        if (!_longFired && (millis() - _downTime) > LONG_MS) {
            _longFired = true;
            ev.type = TouchEvent::LONG_PRESS;
            ev.x = cur.x; ev.y = cur.y;
        } else {
            ev.type = TouchEvent::MOVE;
            ev.x = cur.x; ev.y = cur.y;
            ev.dx = cur.x - _prev.x;
            ev.dy = cur.y - _prev.y;
        }
    } else if (!cur.pressed && _wasDown) {
        // Finger up — classify gesture
        int16_t dx = _prev.x - _downX;
        int16_t dy = _prev.y - _downY;
        uint32_t dt = millis() - _downTime;

        if (abs(dx) > SWIPE_THRESH || abs(dy) > SWIPE_THRESH) {
            if (abs(dx) >= abs(dy)) {
                ev.type = dx > 0 ? TouchEvent::SWIPE_RIGHT : TouchEvent::SWIPE_LEFT;
            } else {
                ev.type = dy > 0 ? TouchEvent::SWIPE_DOWN : TouchEvent::SWIPE_UP;
            }
            ev.dx = dx; ev.dy = dy;
        } else if (dt < 400 && !_longFired) {
            ev.type = TouchEvent::TAP;
        } else {
            ev.type = TouchEvent::UP;
        }
        ev.x = _prev.x; ev.y = _prev.y;
    }

    _prev    = cur;
    _wasDown = cur.pressed;
    return ev;
}
