#include "clock_app.h"
#include "../ui/canvas.h"
#include "../os/appmanager.h"
#include <time.h>
#include <math.h>

static const char* DAYS[]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char* MONTHS[]  = {"January","February","March","April","May","June",
                                  "July","August","September","October","November","December"};

void ClockApp::onEnter() { draw(); }
void ClockApp::tick()    { draw(); }

void ClockApp::drawAnalog() {
    Canvas& cv = Canvas::instance();
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);

    int cx = EPD_W / 2, cy = STATUS_BAR_H + (EPD_H - STATUS_BAR_H - DOCK_H) / 2;
    int r  = min(EPD_W, EPD_H - STATUS_BAR_H - DOCK_H) / 2 - 20;

    // Outer ring
    cv.drawCircle(cx, cy, r, C_BLACK);
    cv.drawCircle(cx, cy, r+1, C_MID);

    // Hour marks
    for (int i = 0; i < 12; i++) {
        float angle = i * 30.0f * M_PI / 180.0f;
        int x1 = cx + (int)((r-6)  * sin(angle));
        int y1 = cy - (int)((r-6)  * cos(angle));
        int x2 = cx + (int)((r-16) * sin(angle));
        int y2 = cy - (int)((r-16) * cos(angle));
        cv.drawLine(x1, y1, x2, y2, C_BLACK);
    }

    // Minute hand
    float mAngle = (t->tm_min * 6.0f + t->tm_sec * 0.1f) * M_PI / 180.0f;
    cv.drawLine(cx, cy,
        cx + (int)((r-20) * sin(mAngle)),
        cy - (int)((r-20) * cos(mAngle)), C_BLACK);

    // Hour hand
    float hAngle = ((t->tm_hour % 12) * 30.0f + t->tm_min * 0.5f) * M_PI / 180.0f;
    cv.drawLine(cx, cy,
        cx + (int)((r-50) * sin(hAngle)),
        cy - (int)((r-50) * cos(hAngle)), C_BLACK);
    // Thick hour hand
    cv.drawLine(cx+1, cy,
        cx+1 + (int)((r-50) * sin(hAngle)),
        cy   - (int)((r-50) * cos(hAngle)), C_BLACK);

    // Second hand
    float sAngle = t->tm_sec * 6.0f * M_PI / 180.0f;
    cv.drawLine(cx, cy,
        cx + (int)((r-10) * sin(sAngle)),
        cy - (int)((r-10) * cos(sAngle)), C_MID);

    // Center dot
    cv.fillCircle(cx, cy, 5, C_BLACK);

    // Date below
    char dateBuf[32];
    snprintf(dateBuf, sizeof(dateBuf), "%s %d %s %d",
        DAYS[t->tm_wday], t->tm_mday, MONTHS[t->tm_mon], 1900+t->tm_year);
    cv.drawTextCentered(cx, cy + r + 16, EPD_W, dateBuf, C_MID, C_WHITE, 1);
}

void ClockApp::drawDigital() {
    Canvas& cv = Canvas::instance();
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);

    int midX = EPD_W / 2;
    int midY = STATUS_BAR_H + (EPD_H - STATUS_BAR_H - DOCK_H) / 2;

    // Big time
    char timeBuf[8];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t->tm_hour, t->tm_min);
    int tw = strlen(timeBuf) * 6 * 6;  // scale 6
    cv.drawText(midX - tw/2, midY - 28, timeBuf, C_BLACK, C_WHITE, 6);

    // Seconds
    char secBuf[4];
    snprintf(secBuf, sizeof(secBuf), ":%02d", t->tm_sec);
    cv.drawText(midX + tw/2 - 10, midY - 6, secBuf, C_MID, C_WHITE, 3);

    // Date
    char dateBuf[40];
    snprintf(dateBuf, sizeof(dateBuf), "%s, %d %s %d",
        DAYS[t->tm_wday], t->tm_mday, MONTHS[t->tm_mon], 1900+t->tm_year);
    cv.drawTextCentered(midX, midY + 50, EPD_W, dateBuf, C_MID, C_WHITE, 2);

    // UTC time smaller
    time_t utcNow = now;  // already UTC from time()
    struct tm* utc = gmtime(&utcNow);
    char utcBuf[16];
    snprintf(utcBuf, sizeof(utcBuf), "UTC %02d:%02d", utc->tm_hour, utc->tm_min);
    cv.drawTextCentered(midX, midY + 80, EPD_W, utcBuf, C_LIGHT, C_WHITE, 1);
}

void ClockApp::drawCalendar() {
    Canvas& cv = Canvas::instance();
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);

    int y = STATUS_BAR_H + 10;
    char header[32];
    snprintf(header, sizeof(header), "%s %d", MONTHS[t->tm_mon], 1900 + t->tm_year);
    cv.drawTextCentered(EPD_W/2, y, EPD_W, header, C_BLACK, C_WHITE, 2);
    y += 30;

    // Day headers
    const char* dow[] = {"Su","Mo","Tu","We","Th","Fr","Sa"};
    int cellW = EPD_W / 7;
    for (int d = 0; d < 7; d++) {
        cv.drawTextCentered(d * cellW + cellW/2, y, cellW, dow[d], C_MID, C_WHITE, 1);
    }
    y += 18;
    cv.drawHLine(0, y, EPD_W, C_LIGHT);
    y += 4;

    // First day of month
    struct tm first = *t;
    first.tm_mday = 1;
    mktime(&first);
    int startDow = first.tm_wday;

    // Days in month
    struct tm next = first;
    next.tm_mon++;
    mktime(&next);
    next.tm_mday = 0;
    mktime(&next);
    int daysInMonth = next.tm_mday;

    int col = startDow;
    int cellH = 46;

    for (int day = 1; day <= daysInMonth; day++) {
        int x  = col * cellW;
        bool today = (day == t->tm_mday);

        if (today) {
            cv.fillCircle(x + cellW/2, y + cellH/2, cellH/2 - 2, C_BLACK);
            char ds[4]; snprintf(ds, sizeof(ds), "%d", day);
            cv.drawTextCentered(x + cellW/2, y + cellH/2 - 4, cellW, ds, C_WHITE, C_BLACK, 1);
        } else {
            char ds[4]; snprintf(ds, sizeof(ds), "%d", day);
            cv.drawTextCentered(x + cellW/2, y + cellH/2 - 4, cellW, ds, C_BLACK, C_WHITE, 1);
        }

        col++;
        if (col == 7) { col = 0; y += cellH; }
    }
}

void ClockApp::draw() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Clock");

    // Tab buttons
    int tabY = STATUS_BAR_H + 4;
    const char* tabs[] = {"Analog","Digital","Calendar"};
    int tabW = EPD_W / 3;
    for (int i = 0; i < 3; i++) {
        bool sel = (i == (int)_view);
        cv.fillRect(i * tabW, tabY, tabW, 28, sel ? C_MID : C_WHITE);
        cv.drawTextCentered(i*tabW + tabW/2, tabY + 8, tabW, tabs[i], C_BLACK, sel ? C_MID : C_WHITE, 1);
    }
    cv.drawHLine(0, tabY + 28, EPD_W, C_MID);

    switch (_view) {
        case VIEW_ANALOG:   drawAnalog();   break;
        case VIEW_DIGITAL:  drawDigital();  break;
        case VIEW_CALENDAR: drawCalendar(); break;
    }

    cv.flushFull();
}

void ClockApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { AppManager::instance().goHome(); return; }

    if (ev.type == TouchEvent::TAP) {
        int tabY = STATUS_BAR_H + 4;
        if (ev.y >= tabY && ev.y < tabY + 28) {
            _view = (View)(ev.x / (EPD_W / 3));
            draw();
            return;
        }
    }
    if (ev.type == TouchEvent::SWIPE_LEFT) {
        _view = (View)(((int)_view + 1) % 3);
        draw();
    }
    if (ev.type == TouchEvent::SWIPE_RIGHT) {
        _view = (View)(((int)_view + 2) % 3);
        draw();
    }
}
