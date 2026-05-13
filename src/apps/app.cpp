#include "app.h"
#include "../ui/canvas.h"
#include "../config.h"

void App::drawNavBar(const char* title) {
    Canvas& cv = Canvas::instance();
    cv.fillRect(0, 0, EPD_W, STATUS_BAR_H, C_BLACK);

    // Back button "< Home"
    cv.drawText(10, (STATUS_BAR_H - 8) / 2, "< Home", C_WHITE, C_BLACK, 1);

    // Title centered
    cv.drawTextCentered(EPD_W / 2, (STATUS_BAR_H - 16) / 2, 400, title, C_WHITE, C_BLACK, 2);
}

bool App::isBackTap(const TouchEvent& ev) {
    return ev.type == TouchEvent::TAP && ev.x < 80 && ev.y < STATUS_BAR_H;
}
