#include "sketch.h"
#include "../ui/canvas.h"
#include "../hal/epd.h"
#include "../hal/storage.h"
#include "../os/appmanager.h"
#include <SD.h>

void SketchApp::onEnter() {
    _pen = 4;
    _points = 0;
    _status[0] = 0;
    clearCanvas();
}

void SketchApp::drawToolbar() {
    Canvas& cv = Canvas::instance();
    cv.fillRect(0, TOOLBAR_Y, EPD_W, TOOLBAR_H, C_LIGHT);

    cv.fillRoundRect(10,  TOOLBAR_Y+4, 110, TOOLBAR_H-8, 6, C_MID);
    cv.drawTextCentered(65, TOOLBAR_Y+12, 110, "Clear", C_BLACK, C_MID, 2);

    cv.fillRoundRect(130, TOOLBAR_Y+4, 110, TOOLBAR_H-8, 6, C_MID);
    cv.drawTextCentered(185, TOOLBAR_Y+12, 110, "Save", C_BLACK, C_MID, 2);

    // pen sizes
    cv.drawText(270, TOOLBAR_Y+12, "Pen:", C_BLACK, C_LIGHT, 2);
    int sizes[] = {2, 4, 7, 11};
    for (int i = 0; i < 4; i++) {
        int bx = 340 + i*64;
        bool sel = (_pen == sizes[i]);
        cv.fillRoundRect(bx, TOOLBAR_Y+4, 56, TOOLBAR_H-8, 6, sel ? C_DARK : C_WHITE);
        cv.fillCircle(bx+28, TOOLBAR_Y+TOOLBAR_H/2, sizes[i], sel ? C_WHITE : C_BLACK);
    }

    if (_status[0])
        cv.drawText(620, TOOLBAR_Y+12, _status, C_BLACK, C_LIGHT, 1);
}

void SketchApp::clearCanvas() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Sketch");
    drawToolbar();
    // draw area border
    cv.drawRect(0, DRAW_Y, EPD_W, EPD_H - DRAW_Y, C_LIGHT);
    cv.flushFull();
}

void SketchApp::draw() {
    clearCanvas();
}

void SketchApp::flushStroke() {
    if (_points == 0) return;
    int x = max(0, _minX - _pen), y = max(0, _minY - _pen);
    int w = min(EPD_W, _maxX + _pen) - x;
    int h = min(EPD_H, _maxY + _pen) - y;
    if (w > 0 && h > 0)
        EPD::instance().flushRegion(Canvas::instance().buf(), x, y, w, h, EPD_DU);
    _points = 0;
}

// Minimal 1-bpp Windows BMP writer (pixel < 8 -> black).
bool SketchApp::saveBMP() {
    if (!Storage::instance().mounted()) { strcpy(_status, "no SD"); return false; }
    char path[64];
    snprintf(path, sizeof(path), "%s/sk_%lu.bmp", SKETCH_PATH, millis());
    File f = SD.open(path, FILE_WRITE);
    if (!f) { strcpy(_status, "save failed"); return false; }

    const int W = EPD_W, H = EPD_H;
    int rowBytes = ((W + 31) / 32) * 4;      // padded to 4 bytes
    uint32_t dataSize = (uint32_t)rowBytes * H;
    uint32_t off = 14 + 40 + 8;              // headers + 2-color palette
    uint32_t fileSize = off + dataSize;

    auto w32 = [&](uint32_t v){ uint8_t b[4]={(uint8_t)v,(uint8_t)(v>>8),(uint8_t)(v>>16),(uint8_t)(v>>24)}; f.write(b,4); };
    auto w16 = [&](uint16_t v){ uint8_t b[2]={(uint8_t)v,(uint8_t)(v>>8)}; f.write(b,2); };

    f.write((const uint8_t*)"BM", 2);
    w32(fileSize); w16(0); w16(0); w32(off);
    w32(40); w32(W); w32(H); w16(1); w16(1); w32(0); w32(dataSize);
    w32(2835); w32(2835); w32(2); w32(0);
    w32(0x00000000); w32(0x00FFFFFF);        // palette: 0=black, 1=white

    const uint8_t* fb = Canvas::instance().buf();
    uint8_t* row = (uint8_t*)malloc(rowBytes);
    if (!row) { f.close(); strcpy(_status, "no mem"); return false; }
    for (int y = H - 1; y >= 0; y--) {       // BMP is bottom-up
        memset(row, 0, rowBytes);
        const uint8_t* src = fb + (size_t)y * W;
        for (int x = 0; x < W; x++) {
            if ((src[x] & 0x0F) >= 8) row[x >> 3] |= (0x80 >> (x & 7)); // white bit
        }
        f.write(row, rowBytes);
    }
    free(row);
    f.close();
    snprintf(_status, sizeof(_status), "saved %s", strrchr(path, '/') + 1);
    return true;
}

void SketchApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { flushStroke(); AppManager::instance().goHome(); return; }

    // Toolbar taps
    if (ev.type == TouchEvent::TAP && ev.y >= TOOLBAR_Y && ev.y < TOOLBAR_Y + TOOLBAR_H) {
        if (ev.x >= 10 && ev.x < 120)  { clearCanvas(); return; }
        if (ev.x >= 130 && ev.x < 240) { saveBMP(); drawToolbar();
            EPD::instance().flushRegion(Canvas::instance().buf(), 0, TOOLBAR_Y, EPD_W, TOOLBAR_H, EPD_DU); return; }
        int sizes[] = {2, 4, 7, 11};
        for (int i = 0; i < 4; i++) {
            int bx = 340 + i*64;
            if (ev.x >= bx && ev.x < bx+56) {
                _pen = sizes[i]; drawToolbar();
                EPD::instance().flushRegion(Canvas::instance().buf(), 0, TOOLBAR_Y, EPD_W, TOOLBAR_H, EPD_DU);
                return;
            }
        }
        return;
    }

    // Drawing
    Canvas& cv = Canvas::instance();
    if (ev.y < DRAW_Y + 2) return;

    if (ev.type == TouchEvent::DOWN || ev.type == TouchEvent::TAP) {
        cv.fillCircle(ev.x, ev.y, _pen, C_BLACK);
        _minX = _maxX = ev.x; _minY = _maxY = ev.y; _points = 1;
        if (ev.type == TouchEvent::TAP) flushStroke();
    } else if (ev.type == TouchEvent::MOVE) {
        int px = ev.x - ev.dx, py = ev.y - ev.dy;
        // draw a thick line by stamping circles along the segment
        int steps = max(abs(ev.dx), abs(ev.dy));
        for (int s = 0; s <= steps; s++) {
            int x = px + (ev.dx * s) / max(1, steps);
            int y = py + (ev.dy * s) / max(1, steps);
            if (y >= DRAW_Y) cv.fillCircle(x, y, _pen, C_BLACK);
        }
        _minX = min(_minX, min(px, (int)ev.x)); _maxX = max(_maxX, max(px, (int)ev.x));
        _minY = min(_minY, min(py, (int)ev.y)); _maxY = max(_maxY, max(py, (int)ev.y));
        if (++_points >= 14) flushStroke();     // periodic coarse refresh
    } else if (ev.type == TouchEvent::UP || ev.type == TouchEvent::LONG_PRESS) {
        flushStroke();
    }
}
