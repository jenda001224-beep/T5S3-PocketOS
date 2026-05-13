#pragma once
#include "app.h"

#define CALC_APP_ID  9

class CalculatorApp : public App {
public:
    CalculatorApp() : App("Calculator", CALC_APP_ID) {}
    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    void press(const char* key);
    void calculate();
    void drawDisplay();
    void drawKeypad();

    char    _display[32]  = "0";
    char    _operand1[32] = "";
    char    _operand2[32] = "";
    char    _op           = 0;
    bool    _newNum       = true;
    bool    _hasResult    = false;

    static const char* KEYS[];
    static constexpr int COLS = 4;
    static constexpr int ROWS = 5;
};
