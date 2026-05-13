#pragma once
#include "app.h"

#define STRATEGY_APP_ID  5

// Turn-based hex strategy game (Polytopia-like)
// Hexagonal grid, 2 players, simple unit types

enum HexTerrain : uint8_t { GRASS=0, FOREST=1, MOUNTAIN=2, WATER=3, CITY=4 };
enum UnitType   : uint8_t { UNIT_NONE=0, WARRIOR=1, ARCHER=2, KNIGHT=3, CATAPULT=4 };

struct HexCell {
    HexTerrain terrain;
    UnitType   unit;
    uint8_t    player;    // 0=none, 1=P1, 2=P2
    uint8_t    unitHP;
    bool       unitMoved;
    bool       city;
    uint8_t    cityLevel;
};

class StrategyApp : public App {
public:
    StrategyApp() : App("Conquest", STRATEGY_APP_ID) {}

    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    static constexpr int MAP_W = 12;
    static constexpr int MAP_H = 9;
    static constexpr int HEX_W = 70;
    static constexpr int HEX_H = 60;

    void newGame();
    void drawHexGrid();
    void drawHex(int q, int r, const HexCell& cell, bool selected, bool reachable);
    void drawUI();
    void endTurn();
    void selectHex(int q, int r);
    void moveUnit(int fq, int fr, int tq, int tr);
    bool canMoveTo(int fq, int fr, int tq, int tr);
    void hexToPixel(int q, int r, int& px, int& py);
    bool pixelToHex(int px, int py, int& q, int& r);
    void aiTurn();
    int  hexDist(int q1, int r1, int q2, int r2);

    HexCell _map[MAP_W][MAP_H];
    int     _currentPlayer = 1;
    int     _turn          = 1;
    int     _gold[3]       = {0, 10, 10};
    int     _score[3]      = {0, 0, 0};
    bool    _gameOver      = false;

    int     _selQ = -1, _selR = -1;
    bool    _selected = false;
    int     _camX = 0, _camY = 0;

    static constexpr int MAP_OFFSET_X = 20;
    static constexpr int MAP_OFFSET_Y = STATUS_BAR_H + 20;
};
