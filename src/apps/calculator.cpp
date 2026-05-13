#include "calculator.h"
#include "../ui/canvas.h"
#include "../os/appmanager.h"
#include <stdlib.h>
#include <math.h>

const char* CalculatorApp::KEYS[] = {
    "C",  "+/-", "%", "/",
    "7",  "8",   "9", "*",
    "4",  "5",   "6", "-",
    "1",  "2",   "3", "+",
    "0",  ".",   "=", "="
};

void CalculatorApp::onEnter() {
    strcpy(_display, "0");
    _op      = 0;
    _newNum  = true;
    draw();
}

void CalculatorApp::press(const char* key) {
    if (strcmp(key, "C") == 0) {
        strcpy(_display, "0");
        strcpy(_operand1, "");
        _op = 0; _newNum = true;
    } else if (strcmp(key, "+/-") == 0) {
        if (_display[0] == '-') memmove(_display, _display+1, strlen(_display));
        else { memmove(_display+1, _display, strlen(_display)+1); _display[0]='-'; }
    } else if (strcmp(key, "%") == 0) {
        double v = atof(_display) / 100.0;
        snprintf(_display, sizeof(_display), "%g", v);
    } else if (strcmp(key, "=") == 0) {
        calculate();
    } else if (strlen(key) == 1 && strchr("+-*/", key[0])) {
        strncpy(_operand1, _display, 31);
        _op = key[0];
        _newNum = true;
    } else {
        // Digit or dot
        if (_newNum) { strcpy(_display, key); _newNum = false; }
        else if (strlen(_display) < 15) strncat(_display, key, 1);
    }
    draw();
}

void CalculatorApp::calculate() {
    if (!_op) return;
    double a = atof(_operand1);
    double b = atof(_display);
    double r = 0;
    switch (_op) {
        case '+': r = a + b; break;
        case '-': r = a - b; break;
        case '*': r = a * b; break;
        case '/': r = (b != 0) ? a / b : 0; break;
    }
    snprintf(_display, sizeof(_display), "%g", r);
    _op = 0; _newNum = true;
}

void CalculatorApp::drawDisplay() {
    Canvas& cv = Canvas::instance();
    int dispH = 100;
    cv.fillRect(0, STATUS_BAR_H, EPD_W, dispH, C_BLACK);

    // Operator
    if (_op) {
        char opStr[2] = {_op, 0};
        cv.drawText(10, STATUS_BAR_H + 10, opStr, C_MID, C_BLACK, 2);
        cv.drawText(30, STATUS_BAR_H + 10, _operand1, C_MID, C_BLACK, 2);
    }

    // Main number — large, right-aligned
    int tw = strlen(_display) * 6 * 4;
    cv.drawText(EPD_W - tw - 16, STATUS_BAR_H + 50, _display, C_WHITE, C_BLACK, 4);
}

void CalculatorApp::drawKeypad() {
    Canvas& cv = Canvas::instance();
    int padY  = STATUS_BAR_H + 100;
    int padH  = EPD_H - padY - DOCK_H;
    int keyW  = EPD_W / COLS;
    int keyH  = padH / ROWS;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            int idx = r * COLS + c;
            if (idx >= ROWS*COLS) break;
            // Last row: "0" spans 2 cols
            int kx = c * keyW;
            int ky = padY + r * keyH;
            int kw = keyW;
            if (r == ROWS-1 && c == 0) kw = keyW * 2;
            if (r == ROWS-1 && c == 1) continue;  // covered by wide 0

            const char* k = KEYS[idx];
            bool isOp  = (strlen(k)==1 && strchr("+-*/=", k[0]));
            bool isClr = (strcmp(k,"C")==0);
            uint8_t bg = isOp ? C_MID : isClr ? C_DARK : C_LIGHT;
            uint8_t fg = (bg == C_DARK) ? C_WHITE : C_BLACK;

            cv.fillRoundRect(kx+2, ky+2, kw-4, keyH-4, 8, bg);
            cv.drawTextCentered(kx + kw/2, ky + keyH/2 - 8, kw, k, fg, bg, 2);
        }
    }
}

void CalculatorApp::draw() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Calculator");
    drawDisplay();
    drawKeypad();
    cv.flushFull();
}

void CalculatorApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { AppManager::instance().goHome(); return; }

    if (ev.type == TouchEvent::TAP) {
        int padY = STATUS_BAR_H + 100;
        int padH = EPD_H - padY - DOCK_H;
        int keyW = EPD_W / COLS;
        int keyH = padH / ROWS;

        int row = (ev.y - padY) / keyH;
        int col = ev.x / keyW;

        if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
            // Handle wide 0 button
            if (row == ROWS-1 && col <= 1) col = 0;
            int idx = row * COLS + col;
            if (idx < ROWS*COLS) press(KEYS[idx]);
        }
    }
}
