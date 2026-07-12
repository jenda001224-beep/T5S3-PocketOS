#pragma once
#include "app.h"

#define SUDOKU_APP_ID  13

class SudokuApp : public App {
public:
    SudokuApp() : App("Sudoku", SUDOKU_APP_ID) {}
    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    uint8_t _board[9][9];
    uint8_t _sol[9][9];
    bool    _given[9][9];
    int     _selR = -1, _selC = -1;
    const char* _msg = "";

    void newGame();
    bool fillBoard(int pos);
    bool valid(int r, int c, int v);
    void place(int v);
    bool solvedOK();
    int  cellSize() const;
    int  gridX() const { return 20; }
    int  gridY() const { return STATUS_BAR_H + 12; }
};
