#pragma once
#include <Wire.h>
#include "../config.h"

struct TouchPoint {
    int16_t x, y;
    bool    pressed;
};

struct TouchEvent {
    enum Type { NONE, DOWN, UP, MOVE, SWIPE_DOWN, SWIPE_UP, SWIPE_LEFT, SWIPE_RIGHT, TAP, LONG_PRESS };
    Type    type     = NONE;
    int16_t x        = 0;
    int16_t y        = 0;
    int16_t dx       = 0;  // swipe delta
    int16_t dy       = 0;
};

class Touch {
public:
    static Touch& instance();
    bool       begin();
    TouchEvent poll();          // call every frame

private:
    Touch() = default;
    bool    readGT911(uint8_t* buf, uint16_t reg, uint8_t len);
    void    writeGT911(uint16_t reg, uint8_t val);
    bool    probe(uint8_t addr);
    void    hwReset(bool pickPrimaryAddr);

    uint8_t _addr = TOUCH_ADDR;

    TouchPoint _prev;
    uint32_t   _downTime    = 0;
    int16_t    _downX       = 0;
    int16_t    _downY       = 0;
    bool       _wasDown     = false;
    bool       _longFired   = false;

    static constexpr int16_t SWIPE_THRESH = 50;
    static constexpr uint32_t LONG_MS     = 600;
};
