#include "canvas.h"
#include "../hal/epd.h"
#include <esp_heap_caps.h>
#include <stdlib.h>
#include <math.h>

Canvas& Canvas::instance() {
    static Canvas inst;
    return inst;
}

bool Canvas::begin() {
    _fb = (uint8_t*)heap_caps_malloc((size_t)EPD_W * EPD_H, MALLOC_CAP_SPIRAM);
    if (!_fb) return false;
    memset(_fb, C_WHITE, (size_t)EPD_W * EPD_H);
    return true;
}

void Canvas::markDirty(int x, int y, int w, int h) {
    _dirtyX1  = min(_dirtyX1, x);
    _dirtyY1  = min(_dirtyY1, y);
    _dirtyX2  = max(_dirtyX2, x + w);
    _dirtyY2  = max(_dirtyY2, y + h);
    _hasDirty = true;
}

void Canvas::resetDirty() {
    _dirtyX1 = EPD_W; _dirtyY1 = EPD_H;
    _dirtyX2 = 0;     _dirtyY2 = 0;
    _hasDirty = false;
}

void Canvas::flushDirty() {
    if (!_hasDirty) return;
    int x = max(0, _dirtyX1);
    int y = max(0, _dirtyY1);
    int w = min(EPD_W, _dirtyX2) - x;
    int h = min(EPD_H, _dirtyY2) - y;
    if (w > 0 && h > 0)
        EPD::instance().flushRegion(_fb, x, y, w, h, EPD_DU);
    resetDirty();
}

void Canvas::flushFull() {
    EPD::instance().flushFull(_fb, EPD_GC16);
    resetDirty();
}

void Canvas::clear(uint8_t c) {
    memset(_fb, c, (size_t)EPD_W * EPD_H);
    markDirty(0, 0, EPD_W, EPD_H);
}

void Canvas::drawPixel(int x, int y, uint8_t c) {
    if (!inBounds(x, y)) return;
    _fb[(size_t)y * EPD_W + x] = c & 0x0F;
    markDirty(x, y, 1, 1);
}

void Canvas::drawHLine(int x, int y, int w, uint8_t c) {
    if (y < 0 || y >= EPD_H) return;
    int x0 = max(0, x), x1 = min(EPD_W, x + w);
    if (x0 >= x1) return;
    memset(_fb + (size_t)y * EPD_W + x0, c & 0x0F, x1 - x0);
    markDirty(x0, y, x1 - x0, 1);
}

void Canvas::drawVLine(int x, int y, int h, uint8_t c) {
    if (x < 0 || x >= EPD_W) return;
    int y0 = max(0, y), y1 = min(EPD_H, y + h);
    for (int i = y0; i < y1; i++) _fb[(size_t)i * EPD_W + x] = c & 0x0F;
    markDirty(x, y0, 1, y1 - y0);
}

void Canvas::drawLine(int x0, int y0, int x1, int y1, uint8_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        drawPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Canvas::drawRect(int x, int y, int w, int h, uint8_t c) {
    drawHLine(x, y,         w, c);
    drawHLine(x, y + h - 1, w, c);
    drawVLine(x,         y, h, c);
    drawVLine(x + w - 1, y, h, c);
}

void Canvas::fillRect(int x, int y, int w, int h, uint8_t c) {
    int x0 = max(0, x), x1 = min(EPD_W, x + w);
    int y0 = max(0, y), y1 = min(EPD_H, y + h);
    if (x0 >= x1 || y0 >= y1) return;
    for (int row = y0; row < y1; row++)
        memset(_fb + (size_t)row * EPD_W + x0, c & 0x0F, x1 - x0);
    markDirty(x0, y0, x1 - x0, y1 - y0);
}

void Canvas::circleHelper(int cx, int cy, int r, uint8_t c, bool fill) {
    int x = 0, y = r, d = 3 - 2 * r;
    while (x <= y) {
        if (fill) {
            drawHLine(cx - x, cy - y, 2 * x + 1, c);
            drawHLine(cx - x, cy + y, 2 * x + 1, c);
            drawHLine(cx - y, cy - x, 2 * y + 1, c);
            drawHLine(cx - y, cy + x, 2 * y + 1, c);
        } else {
            auto p = [&](int px, int py) { drawPixel(px, py, c); };
            p(cx+x,cy+y); p(cx-x,cy+y); p(cx+x,cy-y); p(cx-x,cy-y);
            p(cx+y,cy+x); p(cx-y,cy+x); p(cx+y,cy-x); p(cx-y,cy-x);
        }
        if (d < 0) d += 4 * x + 6;
        else      { d += 4 * (x - y) + 10; y--; }
        x++;
    }
}

void Canvas::drawCircle(int cx, int cy, int r, uint8_t c) { circleHelper(cx, cy, r, c, false); }
void Canvas::fillCircle(int cx, int cy, int r, uint8_t c) { circleHelper(cx, cy, r, c, true); }

void Canvas::fillRoundRect(int x, int y, int w, int h, int r, uint8_t c) {
    fillRect(x + r, y,     w - 2*r, h,     c);
    fillRect(x,     y + r, r,       h-2*r, c);
    fillRect(x+w-r, y + r, r,       h-2*r, c);
    fillCircle(x+r,     y+r,     r, c);
    fillCircle(x+w-r-1, y+r,     r, c);
    fillCircle(x+r,     y+h-r-1, r, c);
    fillCircle(x+w-r-1, y+h-r-1, r, c);
}

void Canvas::drawRoundRect(int x, int y, int w, int h, int r, uint8_t c) {
    drawHLine(x + r, y,         w - 2*r, c);
    drawHLine(x + r, y + h - 1, w - 2*r, c);
    drawVLine(x,         y + r, h - 2*r, c);
    drawVLine(x + w - 1, y + r, h - 2*r, c);
    // corners via arc approximation (simplified)
    int xr = x + r, yr = y + r;
    int x2 = x + w - r - 1, y2 = y + h - r - 1;
    int xx = 0, yy = r, d = 3 - 2 * r;
    while (xx <= yy) {
        drawPixel(xr - xx, yr - yy, c); drawPixel(x2 + xx, yr - yy, c);
        drawPixel(xr - yy, yr - xx, c); drawPixel(x2 + yy, yr - xx, c);
        drawPixel(xr - xx, y2 + yy, c); drawPixel(x2 + xx, y2 + yy, c);
        drawPixel(xr - yy, y2 + xx, c); drawPixel(x2 + yy, y2 + xx, c);
        if (d < 0) d += 4 * xx + 6;
        else { d += 4 * (xx - yy) + 10; yy--; }
        xx++;
    }
}

void Canvas::drawChar(int x, int y, char ch, uint8_t c, uint8_t bg, uint8_t scale) {
    if (ch < 32 || ch > 127) ch = '?';
    const uint8_t* glyph = FONT6x8[ch - 32];
    for (int col = 0; col < 6; col++) {
        uint8_t col_data = glyph[col];
        for (int row = 0; row < 8; row++) {
            uint8_t color = (col_data & (1 << row)) ? c : bg;
            if (scale == 1) {
                drawPixel(x + col, y + row, color);
            } else {
                fillRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

int Canvas::drawText(int x, int y, const char* s, uint8_t c, uint8_t bg, uint8_t scale) {
    int cx = x;
    while (*s) {
        if (*s == '\n') { cx = x; y += 8 * scale; s++; continue; }
        drawChar(cx, y, *s++, c, bg, scale);
        cx += 6 * scale;
    }
    return cx;
}

int Canvas::drawTextCentered(int cx, int y, int maxW, const char* s, uint8_t c, uint8_t bg, uint8_t scale) {
    int tw = textWidth(s, scale);
    int x  = cx - tw / 2;
    return drawText(x, y, s, c, bg, scale);
}

int Canvas::textWidth(const char* s, uint8_t scale) {
    int len = 0;
    while (*s++) len++;
    return len * 6 * scale;
}

void Canvas::drawIcon16(int x, int y, const uint8_t* bmp, uint8_t c, uint8_t bg) {
    for (int row = 0; row < 16; row++) {
        uint16_t bits = ((uint16_t)bmp[row * 2] << 8) | bmp[row * 2 + 1];
        for (int col = 0; col < 16; col++) {
            uint8_t color = (bits & (0x8000 >> col)) ? c : bg;
            drawPixel(x + col, y + row, color);
        }
    }
    markDirty(x, y, 16, 16);
}

void Canvas::drawIconScaled(int x, int y, int size, const uint8_t* bmp, uint8_t c, uint8_t bg) {
    float scale = (float)size / 16.0f;
    for (int row = 0; row < 16; row++) {
        uint16_t bits = ((uint16_t)bmp[row * 2] << 8) | bmp[row * 2 + 1];
        for (int col = 0; col < 16; col++) {
            uint8_t color = (bits & (0x8000 >> col)) ? c : bg;
            int px = x + (int)(col * scale);
            int py = y + (int)(row * scale);
            int ps = max(1, (int)scale);
            fillRect(px, py, ps, ps, color);
        }
    }
    markDirty(x, y, size, size);
}
