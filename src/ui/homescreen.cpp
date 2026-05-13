#include "homescreen.h"
#include "canvas.h"
#include "statusbar.h"
#include <math.h>

HomeScreen& HomeScreen::instance() {
    static HomeScreen inst;
    return inst;
}

void HomeScreen::begin(const AppEntry* apps, int count) {
    _apps  = apps;
    _count = count;
    _page  = 0;
}

int HomeScreen::totalPages() const {
    int grid = _count - DOCK_COUNT;
    if (grid <= 0) return 1;
    return (grid + GRID_COUNT - 1) / GRID_COUNT;
}

void HomeScreen::setPage(int p) {
    _page = constrain(p, 0, totalPages() - 1);
}

// Returns pixel position of slot (grid slot 0..GRID_COUNT-1)
static void slotRect(int slot, int& ox, int& oy) {
    int col = slot % APP_COLS;
    int row = slot / APP_COLS;
    int totalW  = APP_COLS * ICON_SIZE + (APP_COLS - 1) * ICON_PAD;
    int startX  = (EPD_W - totalW) / 2;
    int startY  = STATUS_BAR_H + 24;
    ox = startX + col * (ICON_SIZE + ICON_PAD);
    oy = startY + row * (ICON_SIZE + ICON_PAD + 20);  // 20 for label
}

static void dockSlotRect(int slot, int& ox, int& oy) {
    int dockY    = EPD_H - DOCK_H;
    int totalW   = HomeScreen::DOCK_COUNT * ICON_SIZE + (HomeScreen::DOCK_COUNT - 1) * ICON_PAD;
    int startX   = (EPD_W - totalW) / 2;
    ox = startX + slot * (ICON_SIZE + ICON_PAD);
    oy = dockY + (DOCK_H - ICON_SIZE) / 2;
}

void HomeScreen::drawAppIcon(int slot, const AppEntry& app, bool highlight) {
    Canvas& cv = Canvas::instance();
    int x, y;
    slotRect(slot, x, y);

    uint8_t bg   = highlight ? C_LIGHT : C_WHITE;
    uint8_t fg   = C_BLACK;
    uint8_t ibg  = highlight ? C_MID   : C_LIGHT;

    // Icon background rounded rect
    cv.fillRoundRect(x, y, ICON_SIZE, ICON_SIZE, CORNER_R, ibg);

    // Icon centered in box
    int iconPad = (ICON_SIZE - 48) / 2;   // scale icon to 48×48
    cv.drawIconScaled(x + iconPad, y + iconPad, 48, app.icon, fg, ibg);

    // Label below icon
    cv.drawTextCentered(x + ICON_SIZE / 2, y + ICON_SIZE + 4, ICON_SIZE, app.name, C_BLACK, C_WHITE, 1);
}

void HomeScreen::draw() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);

    // Status bar
    StatusBar::instance().draw();

    // Grid icons
    int offset = pageOffset();
    for (int slot = 0; slot < GRID_COUNT; slot++) {
        int idx = offset + slot;
        if (idx >= _count) break;
        drawAppIcon(slot, _apps[idx], slot == _highlighted);
    }

    // Dock separator line
    int dockY = EPD_H - DOCK_H;
    cv.drawHLine(0, dockY, EPD_W, C_LIGHT);

    // Dock icons (always first DOCK_COUNT apps)
    for (int i = 0; i < DOCK_COUNT && i < _count; i++) {
        int x, y;
        dockSlotRect(i, x, y);
        cv.fillRoundRect(x, y, ICON_SIZE, ICON_SIZE, CORNER_R, C_LIGHT);
        int iconPad = (ICON_SIZE - 48) / 2;
        cv.drawIconScaled(x + iconPad, y + iconPad, 48, _apps[i].icon, C_BLACK, C_LIGHT);
    }

    drawPageDots();
    cv.flushFull();
}

void HomeScreen::drawPageDots() {
    Canvas& cv = Canvas::instance();
    int total = totalPages();
    int spacing = 14;
    int startX  = EPD_W / 2 - (total * spacing) / 2;
    int dotY    = PAGE_DOT_Y;

    for (int i = 0; i < total; i++) {
        int cx = startX + i * spacing + spacing / 2;
        if (i == _page) cv.fillCircle(cx, dotY, 4, C_BLACK);
        else             cv.drawCircle(cx, dotY, 4, C_MID);
    }
}

int HomeScreen::hitTest(int x, int y) {
    // Check grid slots
    for (int slot = 0; slot < GRID_COUNT; slot++) {
        int sx, sy;
        slotRect(slot, sx, sy);
        if (x >= sx && x < sx + ICON_SIZE && y >= sy && y < sy + ICON_SIZE)
            return slot;
    }
    return -1;
}

int HomeScreen::handleEvent(const TouchEvent& ev) {
    if (ev.type == TouchEvent::SWIPE_LEFT) {
        setPage(_page + 1);
        draw();
        return -1;
    }
    if (ev.type == TouchEvent::SWIPE_RIGHT) {
        setPage(_page - 1);
        draw();
        return -1;
    }
    if (ev.type == TouchEvent::TAP) {
        // Check dock
        for (int i = 0; i < DOCK_COUNT && i < _count; i++) {
            int x, y;
            dockSlotRect(i, x, y);
            if (ev.x >= x && ev.x < x + ICON_SIZE && ev.y >= y && ev.y < y + ICON_SIZE)
                return _apps[i].appId;
        }
        // Check grid
        int slot = hitTest(ev.x, ev.y);
        if (slot >= 0) {
            int idx = pageOffset() + slot;
            if (idx < _count) return _apps[idx].appId;
        }
    }
    return -1;
}
