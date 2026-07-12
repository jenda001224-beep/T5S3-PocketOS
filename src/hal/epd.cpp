#include "epd.h"
#include <Arduino.h>
#include <FastEPD.h>

// One static FastEPD instance for the whole system.
static FASTEPD s_epd;

EPD& EPD::instance() {
    static EPD inst;
    return inst;
}

bool EPD::begin() {
    // BB_PANEL_EPDIY_V7 is the profile that matches the T5 E-Paper S3 Pro (H752):
    // ED047 data bus {5,6,7,15,16,17,18,8}, STV45/CKV48/STH41/LEH42/CKH4, I2C 39/40.
    // 26.6 MHz bus clock, exactly as the vendor getting_started example uses.
    int rc = s_epd.initPanel(BB_PANEL_EPDIY_V7, 26666666);
    s_epd.setPanelSize(EPD_W, EPD_H);
    s_epd.setMode(BB_MODE_4BPP);           // 16-level grayscale, 0=black 15=white
    s_epd.fillScreen(C_WHITE);
    s_epd.fullUpdate(CLEAR_SLOW);          // clean the panel once at boot

    _panelW = s_epd.width();
    _panelH = s_epd.height();
    _ok = (rc == BBEP_SUCCESS);

    Serial.printf("[EPD] FastEPD panel %dx%d rc=%d\n", _panelW, _panelH, rc);
    // Even on a non-zero rc we keep going: the UI is usable and this lets the
    // device boot for debugging instead of hanging on a blank screen.
    return true;
}

// Pack Canvas' 8bpp (0-15) framebuffer region into FastEPD's 4bpp buffer.
// Layout for rotation 0: byte = (x>>1) + y*(width/2); even x -> high nibble.
static void copyRegion(const uint8_t* fb, int x, int y, int w, int h) {
    uint8_t* dst = s_epd.currentBuffer();
    if (!dst) return;
    const int pitch = EPD_W >> 1;
    for (int row = y; row < y + h; row++) {
        const uint8_t* src = fb + (size_t)row * EPD_W;
        uint8_t*       d   = dst + (size_t)row * pitch;
        for (int col = x; col < x + w; col++) {
            uint8_t v = src[col] & 0x0F;
            int idx = col >> 1;
            if (col & 1) d[idx] = (d[idx] & 0xF0) | v;
            else         d[idx] = (d[idx] & 0x0F) | (v << 4);
        }
    }
}

void EPD::clear() {
    s_epd.fillScreen(C_WHITE);
    s_epd.fullUpdate(CLEAR_SLOW);
}

void EPD::flushFull(const uint8_t* fb, EPDMode mode) {
    copyRegion(fb, 0, 0, EPD_W, EPD_H);
    // CLEAR_FAST = 8 black/white passes: crisp grayscale full refresh (~1s).
    s_epd.fullUpdate(CLEAR_FAST);
}

void EPD::flushRegion(const uint8_t* fb, int x, int y, int w, int h, EPDMode mode) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > EPD_W) w = EPD_W - x;
    if (y + h > EPD_H) h = EPD_H - y;
    if (w <= 0 || h <= 0) return;
    // even-align x for nibble packing
    if (x & 1) { x--; w++; }
    copyRegion(fb, x, y, w, h);
    BB_RECT r; r.x = x; r.y = y; r.w = w; r.h = h;
    s_epd.fullUpdate(CLEAR_FAST, false, &r);   // regional grayscale update
}

void EPD::setFrontlight(uint8_t brightness) {
    // GPIO11 on this board is the eink DC/DC power enable (driven by FastEPD),
    // NOT a frontlight — so we only remember the value for the UI/ADI app.
    _frontlight = brightness;
}
