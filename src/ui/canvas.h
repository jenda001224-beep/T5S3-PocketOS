#pragma once
#include <stdint.h>
#include <string.h>
#include "../config.h"
#include "fonts.h"

// Canvas wraps a flat 8bpp (0-15 gray) framebuffer in PSRAM.
// Call flush*() to push dirty regions to the EPD.
class Canvas {
public:
    static Canvas& instance();
    bool   begin();   // allocates PSRAM framebuffer

    // ── Primitive drawing ─────────────────────────────────────────────────────
    void   clear(uint8_t c = C_WHITE);
    void   drawPixel(int x, int y, uint8_t c);
    void   drawHLine(int x, int y, int w, uint8_t c);
    void   drawVLine(int x, int y, int h, uint8_t c);
    void   drawLine(int x0, int y0, int x1, int y1, uint8_t c);
    void   drawRect(int x, int y, int w, int h, uint8_t c);
    void   fillRect(int x, int y, int w, int h, uint8_t c);
    void   fillRoundRect(int x, int y, int w, int h, int r, uint8_t c);
    void   drawRoundRect(int x, int y, int w, int h, int r, uint8_t c);
    void   drawCircle(int cx, int cy, int r, uint8_t c);
    void   fillCircle(int cx, int cy, int r, uint8_t c);

    // ── Text ──────────────────────────────────────────────────────────────────
    // scale=1 → 6×8, scale=2 → 12×16, scale=3 → 18×24 …
    void   drawChar(int x, int y, char ch, uint8_t c, uint8_t bg, uint8_t scale = 1);
    int    drawText(int x, int y, const char* s, uint8_t c, uint8_t bg, uint8_t scale = 1);
    int    drawTextCentered(int cx, int y, int maxW, const char* s, uint8_t c, uint8_t bg, uint8_t scale = 1);
    int    textWidth(const char* s, uint8_t scale = 1);

    // ── Icon bitmaps (16×16, 1bpp, stored row-major) ─────────────────────────
    void   drawIcon16(int x, int y, const uint8_t* bmp, uint8_t c, uint8_t bg);
    // Scale a 16×16 icon to arbitrary size
    void   drawIconScaled(int x, int y, int size, const uint8_t* bmp, uint8_t c, uint8_t bg);

    // ── Dirty tracking & flush ────────────────────────────────────────────────
    void   markDirty(int x, int y, int w, int h);
    void   flushDirty();
    void   flushFull();
    void   resetDirty();

    uint8_t* buf() const { return _fb; }

private:
    Canvas() = default;
    void   circleHelper(int cx, int cy, int r, uint8_t c, bool fill);
    inline bool inBounds(int x, int y) { return x >= 0 && x < EPD_W && y >= 0 && y < EPD_H; }

    uint8_t* _fb      = nullptr;
    int      _dirtyX1 = EPD_W, _dirtyY1 = EPD_H;
    int      _dirtyX2 = 0,     _dirtyY2 = 0;
    bool     _hasDirty = false;
};
