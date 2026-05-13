#include "statusbar.h"
#include "canvas.h"
#include <time.h>

StatusBar& StatusBar::instance() {
    static StatusBar inst;
    return inst;
}

void StatusBar::tick() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    snprintf(_timeBuf, sizeof(_timeBuf), "%02d:%02d", t->tm_hour, t->tm_min);
}

void StatusBar::draw() {
    Canvas& cv = Canvas::instance();

    // Background bar
    cv.fillRect(0, 0, EPD_W, STATUS_BAR_H, C_BLACK);

    // Time — centered
    cv.drawTextCentered(EPD_W / 2, (STATUS_BAR_H - 16) / 2, 200, _timeBuf, C_WHITE, C_BLACK, 2);

    // Battery indicator (right side)
    int bx = EPD_W - 50;
    int by = (STATUS_BAR_H - 12) / 2;
    cv.drawRect(bx, by, 28, 12, C_WHITE);
    cv.fillRect(bx + 28, by + 3, 3, 6, C_WHITE);  // nub
    int fill = (int)((_battery / 100.0f) * 24);
    uint8_t col = _battery < 20 ? C_MID : C_WHITE;
    cv.fillRect(bx + 2, by + 2, fill, 8, col);

    // Battery percent text
    char bpct[5]; snprintf(bpct, sizeof(bpct), "%d%%", _battery);
    cv.drawText(bx - 32, by, bpct, C_WHITE, C_BLACK, 1);

    // GPS icon
    int ix = 10;
    if (_gpsFix) {
        cv.drawText(ix, (STATUS_BAR_H - 8) / 2, "GPS", C_WHITE, C_BLACK, 1);
        ix += 24;
    }

    // LoRa icon
    if (_loraActive) {
        cv.drawText(ix, (STATUS_BAR_H - 8) / 2, "LR", C_WHITE, C_BLACK, 1);
        ix += 18;
    }
}
