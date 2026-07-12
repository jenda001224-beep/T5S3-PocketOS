#include "sudoku.h"
#include "../ui/canvas.h"
#include "../os/appmanager.h"
#include <Arduino.h>
#include <esp_system.h>
#include <esp_random.h>

int SudokuApp::cellSize() const {
    int avail = EPD_H - gridY() - DOCK_H - 12;
    return avail / 9;
}

bool SudokuApp::valid(int r, int c, int v) {
    for (int i = 0; i < 9; i++)
        if (_board[r][i] == v || _board[i][c] == v) return false;
    int br = (r/3)*3, bc = (c/3)*3;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (_board[br+i][bc+j] == v) return false;
    return true;
}

bool SudokuApp::fillBoard(int pos) {
    if (pos == 81) return true;
    int r = pos / 9, c = pos % 9;
    uint8_t nums[9] = {1,2,3,4,5,6,7,8,9};
    for (int i = 8; i > 0; i--) { int j = random(i+1); uint8_t t=nums[i]; nums[i]=nums[j]; nums[j]=t; }
    for (int i = 0; i < 9; i++) {
        if (valid(r, c, nums[i])) {
            _board[r][c] = nums[i];
            if (fillBoard(pos+1)) return true;
            _board[r][c] = 0;
        }
    }
    return false;
}

void SudokuApp::newGame() {
    randomSeed(esp_random());
    memset(_board, 0, sizeof(_board));
    fillBoard(0);
    memcpy(_sol, _board, sizeof(_sol));

    // Remove ~48 cells to make the puzzle.
    int remove = 48;
    while (remove > 0) {
        int r = random(9), c = random(9);
        if (_board[r][c] != 0) { _board[r][c] = 0; remove--; }
    }
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++)
            _given[r][c] = (_board[r][c] != 0);

    _selR = _selC = -1;
    _msg = "New puzzle";
}

void SudokuApp::onEnter() { newGame(); draw(); }

bool SudokuApp::solvedOK() {
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++)
            if (_board[r][c] != _sol[r][c]) return false;
    return true;
}

void SudokuApp::place(int v) {
    if (_selR < 0 || _given[_selR][_selC]) return;
    _board[_selR][_selC] = v;
    _msg = "";
    draw();
}

void SudokuApp::draw() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Sudoku");

    int cs = cellSize();
    int gx = gridX(), gy = gridY();

    // cells
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            int x = gx + c*cs, y = gy + r*cs;
            bool sel = (r == _selR && c == _selC);
            cv.fillRect(x, y, cs, cs, sel ? C_LIGHT : C_WHITE);
            if (_board[r][c]) {
                char d[2] = { (char)('0' + _board[r][c]), 0 };
                uint8_t col = _given[r][c] ? C_BLACK : C_DARK;
                cv.drawTextCentered(x + cs/2, y + cs/2 - 8, cs, d, col, sel ? C_LIGHT : C_WHITE, 2);
            }
        }
    }
    // grid lines (thick every 3)
    for (int i = 0; i <= 9; i++) {
        uint8_t col = (i % 3 == 0) ? C_BLACK : C_MID;
        cv.drawHLine(gx, gy + i*cs, 9*cs, col);
        cv.drawVLine(gx + i*cs, gy, 9*cs, col);
        if (i % 3 == 0) {
            cv.drawHLine(gx, gy + i*cs + 1, 9*cs, col);
            cv.drawVLine(gx + i*cs + 1, gy, 9*cs, col);
        }
    }

    // number pad on the right
    int px = gx + 9*cs + 40;
    int pw = 90, ph = 60, gap = 8;
    for (int n = 1; n <= 9; n++) {
        int col = (n-1) % 3, row = (n-1) / 3;
        int bx = px + col*(pw+gap), by = gy + row*(ph+gap);
        cv.fillRoundRect(bx, by, pw, ph, 8, C_LIGHT);
        char d[2] = { (char)('0'+n), 0 };
        cv.drawTextCentered(bx+pw/2, by+ph/2-12, pw, d, C_BLACK, C_LIGHT, 3);
    }
    int by = gy + 3*(ph+gap);
    cv.fillRoundRect(px, by, pw, ph, 8, C_MID);
    cv.drawTextCentered(px+pw/2, by+ph/2-8, pw, "Erase", C_BLACK, C_MID, 1);
    cv.fillRoundRect(px+pw+gap, by, pw, ph, 8, C_MID);
    cv.drawTextCentered(px+pw+gap+pw/2, by+ph/2-8, pw, "Check", C_BLACK, C_MID, 1);
    cv.fillRoundRect(px+2*(pw+gap), by, pw, ph, 8, C_DARK);
    cv.drawTextCentered(px+2*(pw+gap)+pw/2, by+ph/2-8, pw, "New", C_WHITE, C_DARK, 1);

    if (_msg[0]) cv.drawText(px, by + ph + 16, _msg, C_BLACK, C_WHITE, 2);

    cv.flushFull();
}

void SudokuApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { AppManager::instance().goHome(); return; }
    if (ev.type != TouchEvent::TAP) return;

    int cs = cellSize();
    int gx = gridX(), gy = gridY();

    // grid selection
    if (ev.x >= gx && ev.x < gx + 9*cs && ev.y >= gy && ev.y < gy + 9*cs) {
        _selC = (ev.x - gx) / cs;
        _selR = (ev.y - gy) / cs;
        draw();
        return;
    }

    // number pad
    int px = gx + 9*cs + 40;
    int pw = 90, ph = 60, gap = 8;
    for (int n = 1; n <= 9; n++) {
        int col = (n-1) % 3, row = (n-1) / 3;
        int bx = px + col*(pw+gap), by = gy + row*(ph+gap);
        if (ev.x >= bx && ev.x < bx+pw && ev.y >= by && ev.y < by+ph) { place(n); return; }
    }
    int by = gy + 3*(ph+gap);
    if (ev.y >= by && ev.y < by+ph) {
        if (ev.x >= px && ev.x < px+pw) { place(0); return; }
        if (ev.x >= px+pw+gap && ev.x < px+2*pw+gap) {
            bool full = true;
            for (int r=0;r<9;r++) for(int c=0;c<9;c++) if(!_board[r][c]) full=false;
            _msg = !full ? "Keep going…" : (solvedOK() ? "Solved! :)" : "Not correct");
            draw(); return;
        }
        if (ev.x >= px+2*(pw+gap) && ev.x < px+3*pw+2*gap) { newGame(); draw(); return; }
    }
}
