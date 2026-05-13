#include "strategy.h"
#include "../ui/canvas.h"
#include "../os/appmanager.h"
#include <stdlib.h>

void StrategyApp::hexToPixel(int q, int r, int& px, int& py) {
    px = MAP_OFFSET_X + _camX + q * HEX_W + (r % 2) * (HEX_W / 2);
    py = MAP_OFFSET_Y + _camY + r * (HEX_H * 3 / 4);
}

bool StrategyApp::pixelToHex(int px, int py, int& q, int& r) {
    int ry = (py - MAP_OFFSET_Y - _camY) / (HEX_H * 3 / 4);
    int rx = (px - MAP_OFFSET_X - _camX - (ry % 2) * (HEX_W / 2)) / HEX_W;
    q = rx; r = ry;
    return q >= 0 && q < MAP_W && r >= 0 && r < MAP_H;
}

int StrategyApp::hexDist(int q1, int r1, int q2, int r2) {
    return (abs(q1-q2) + abs(q1+r1-q2-r2) + abs(r1-r2)) / 2;
}

void StrategyApp::newGame() {
    // Generate random terrain
    for (int r = 0; r < MAP_H; r++) {
        for (int q = 0; q < MAP_W; q++) {
            int rng = random(10);
            if      (rng < 5) _map[q][r].terrain = GRASS;
            else if (rng < 7) _map[q][r].terrain = FOREST;
            else if (rng < 9) _map[q][r].terrain = MOUNTAIN;
            else              _map[q][r].terrain = WATER;
            _map[q][r].unit       = UNIT_NONE;
            _map[q][r].player     = 0;
            _map[q][r].unitHP     = 0;
            _map[q][r].unitMoved  = false;
            _map[q][r].city       = false;
            _map[q][r].cityLevel  = 0;
        }
    }
    // Place cities
    auto placeCity = [&](int q, int r, int player) {
        _map[q][r].terrain   = GRASS;
        _map[q][r].city      = true;
        _map[q][r].player    = player;
        _map[q][r].cityLevel = 1;
    };
    placeCity(1, 1, 1);
    placeCity(MAP_W-2, MAP_H-2, 2);

    // Starting warriors
    _map[1][2].unit = WARRIOR; _map[1][2].player = 1; _map[1][2].unitHP = 10;
    _map[MAP_W-2][MAP_H-3].unit = WARRIOR; _map[MAP_W-2][MAP_H-3].player = 2; _map[MAP_W-2][MAP_H-3].unitHP = 10;

    _currentPlayer = 1;
    _turn = 1;
    _gold[1] = _gold[2] = 10;
    _score[1] = _score[2] = 0;
    _gameOver = false;
    _selected = false;
}

void StrategyApp::drawHex(int q, int r, const HexCell& cell, bool selected, bool reachable) {
    Canvas& cv = Canvas::instance();
    int px, py;
    hexToPixel(q, r, px, py);

    // Flat-top hexagon approximation with rectangle + triangles
    // Simplified: just draw a rounded rect
    static const uint8_t terrainColor[] = { C_WHITE, C_LIGHT, C_MID, C_DARK, C_LIGHT };
    uint8_t bg = terrainColor[cell.terrain];
    if (selected)  bg = C_MID;
    if (reachable) bg = 11;  // light highlight

    cv.fillRoundRect(px, py, HEX_W-4, HEX_H-4, 6, bg);
    cv.drawRoundRect(px, py, HEX_W-4, HEX_H-4, 6, C_DARK);

    // Terrain label
    static const char* terrainChar[] = {"G","F","M","~","C"};
    cv.drawTextCentered(px + HEX_W/2 - 2, py + 4, HEX_W, terrainChar[cell.terrain], C_DARK, bg, 1);

    // City
    if (cell.city) {
        cv.fillRoundRect(px+4, py+4, HEX_W-12, 16, 3,
            cell.player == 1 ? C_BLACK : C_MID);
        char cname[4]; snprintf(cname, sizeof(cname), "P%d", cell.player);
        cv.drawTextCentered(px+HEX_W/2-2, py+8, HEX_W, cname, C_WHITE,
            cell.player == 1 ? C_BLACK : C_MID, 1);
    }

    // Unit
    if (cell.unit != UNIT_NONE) {
        static const char* unitChar[] = {"","W","A","K","C"};
        uint8_t ufg = cell.player == 1 ? C_WHITE : C_BLACK;
        uint8_t ubg = cell.player == 1 ? C_BLACK : C_LIGHT;
        cv.fillCircle(px + HEX_W/2 - 2, py + HEX_H/2, 14, ubg);
        cv.drawTextCentered(px + HEX_W/2 - 2, py + HEX_H/2 - 4, HEX_W,
            unitChar[cell.unit], ufg, ubg, 1);
        // HP bar
        int barW = (int)((cell.unitHP / 10.0f) * (HEX_W - 12));
        cv.fillRect(px + 4, py + HEX_H - 10, HEX_W-12, 4, C_LIGHT);
        cv.fillRect(px + 4, py + HEX_H - 10, barW, 4, C_BLACK);
        // Moved indicator
        if (cell.unitMoved) cv.drawCircle(px + HEX_W/2-2, py+HEX_H/2, 14, C_MID);
    }
}

void StrategyApp::drawUI() {
    Canvas& cv = Canvas::instance();
    int uiY = EPD_H - DOCK_H - 44;
    cv.fillRect(0, uiY, EPD_W, 44, C_LIGHT);

    // Player indicator
    char turn[32];
    snprintf(turn, sizeof(turn), "Turn %d  Player %d", _turn, _currentPlayer);
    cv.drawText(10, uiY + 6, turn, C_BLACK, C_LIGHT, 1);

    // Gold
    char gold[16]; snprintf(gold, sizeof(gold), "Gold: %d", _gold[_currentPlayer]);
    cv.drawText(10, uiY + 22, gold, C_BLACK, C_LIGHT, 1);

    // Score
    char score[32]; snprintf(score, sizeof(score), "P1:%d  P2:%d", _score[1], _score[2]);
    cv.drawTextCentered(EPD_W/2, uiY + 14, 200, score, C_BLACK, C_LIGHT, 1);

    // End turn button
    cv.fillRoundRect(EPD_W - 110, uiY + 6, 100, 32, 6, C_BLACK);
    cv.drawTextCentered(EPD_W - 60, uiY + 18, 100, "End Turn", C_WHITE, C_BLACK, 1);

    if (_gameOver) {
        cv.fillRoundRect(EPD_W/2 - 150, EPD_H/2 - 40, 300, 80, 12, C_BLACK);
        char win[32]; snprintf(win, sizeof(win), "Player %d Wins!", _score[1]>_score[2] ? 1 : 2);
        cv.drawTextCentered(EPD_W/2, EPD_H/2 - 10, 300, win, C_WHITE, C_BLACK, 2);
    }
}

void StrategyApp::drawHexGrid() {
    Canvas& cv = Canvas::instance();
    cv.fillRect(0, STATUS_BAR_H, EPD_W, EPD_H - STATUS_BAR_H - DOCK_H, C_WHITE);

    for (int r = 0; r < MAP_H; r++) {
        for (int q = 0; q < MAP_W; q++) {
            bool sel  = _selected && _selQ == q && _selR == r;
            bool reach = false;
            if (_selected && _map[_selQ][_selR].unit != UNIT_NONE) {
                reach = hexDist(_selQ, _selR, q, r) <= 2 &&
                        _map[q][r].terrain != WATER;
            }
            drawHex(q, r, _map[q][r], sel, reach);
        }
    }
}

void StrategyApp::draw() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Conquest");
    drawHexGrid();
    drawUI();
    cv.flushFull();
}

void StrategyApp::onEnter() {
    newGame();
    draw();
}

void StrategyApp::endTurn() {
    // Reset unit moved flags for current player
    for (int r = 0; r < MAP_H; r++)
        for (int q = 0; q < MAP_W; q++)
            if (_map[q][r].player == _currentPlayer) _map[q][r].unitMoved = false;

    // Income from cities
    for (int r = 0; r < MAP_H; r++)
        for (int q = 0; q < MAP_W; q++)
            if (_map[q][r].city && _map[q][r].player == _currentPlayer)
                _gold[_currentPlayer] += _map[q][r].cityLevel * 2;

    _currentPlayer = (_currentPlayer == 1) ? 2 : 1;
    if (_currentPlayer == 1) _turn++;

    _selected = false;

    // Simple AI for player 2
    if (_currentPlayer == 2) { aiTurn(); _currentPlayer = 1; }
    draw();
}

void StrategyApp::aiTurn() {
    // Move each unit towards nearest P1 unit or city
    for (int r = 0; r < MAP_H; r++) {
        for (int q = 0; q < MAP_W; q++) {
            if (_map[q][r].player != 2 || _map[q][r].unit == UNIT_NONE || _map[q][r].unitMoved) continue;
            // Find nearest target
            int bestDist = 999, bq = q, br = r;
            for (int tr = 0; tr < MAP_H; tr++) for (int tq = 0; tq < MAP_W; tq++) {
                if (_map[tq][tr].player != 1) continue;
                int d = hexDist(q, r, tq, tr);
                if (d < bestDist) { bestDist = d; bq = tq; br = tr; }
            }
            // Move one step towards target
            int dq = bq - q, dr = br - r;
            int mq = q + (dq > 0 ? 1 : dq < 0 ? -1 : 0);
            int mr = r + (dr > 0 ? 1 : dr < 0 ? -1 : 0);
            if (mq >= 0 && mq < MAP_W && mr >= 0 && mr < MAP_H &&
                _map[mq][mr].terrain != WATER && _map[mq][mr].player != 2) {
                // Attack or move
                if (_map[mq][mr].player == 1 && _map[mq][mr].unit != UNIT_NONE) {
                    _map[mq][mr].unitHP -= 3;
                    if (_map[mq][mr].unitHP <= 0) {
                        _map[mq][mr].unit = UNIT_NONE;
                        _map[mq][mr].player = 0;
                        _score[2]++;
                    }
                } else {
                    _map[mq][mr].unit   = _map[q][r].unit;
                    _map[mq][mr].player = 2;
                    _map[mq][mr].unitHP = _map[q][r].unitHP;
                    _map[q][r].unit = UNIT_NONE;
                    _map[q][r].player = 0;
                }
                _map[mq][mr].unitMoved = true;
            }
        }
    }
    // Reset for AI turn end
    for (int r=0;r<MAP_H;r++) for (int q=0;q<MAP_W;q++)
        if (_map[q][r].player==2) _map[q][r].unitMoved=false;
}

void StrategyApp::selectHex(int q, int r) {
    if (_selected) {
        if (_selQ == q && _selR == r) { _selected = false; draw(); return; }
        // Try to move selected unit
        if (_map[_selQ][_selR].unit != UNIT_NONE &&
            _map[_selQ][_selR].player == _currentPlayer &&
            !_map[_selQ][_selR].unitMoved) {
            if (canMoveTo(_selQ, _selR, q, r)) {
                moveUnit(_selQ, _selR, q, r);
                _selected = false;
                draw();
                return;
            }
        }
        // Reselect
        if (_map[q][r].player == _currentPlayer && _map[q][r].unit != UNIT_NONE) {
            _selQ = q; _selR = r;
        } else {
            _selected = false;
        }
    } else {
        if (_map[q][r].player == _currentPlayer && _map[q][r].unit != UNIT_NONE) {
            _selected = true;
            _selQ = q; _selR = r;
        }
    }
    draw();
}

bool StrategyApp::canMoveTo(int fq, int fr, int tq, int tr) {
    if (tq < 0 || tq >= MAP_W || tr < 0 || tr >= MAP_H) return false;
    if (_map[tq][tr].terrain == WATER) return false;
    if (_map[tq][tr].player == _currentPlayer && _map[tq][tr].unit != UNIT_NONE) return false;
    return hexDist(fq, fr, tq, tr) <= 2;
}

void StrategyApp::moveUnit(int fq, int fr, int tq, int tr) {
    HexCell& src = _map[fq][fr];
    HexCell& dst = _map[tq][tr];

    if (dst.player != _currentPlayer && dst.unit != UNIT_NONE) {
        // Combat
        dst.unitHP -= 4;
        src.unitHP -= 2;
        if (dst.unitHP <= 0) {
            _score[_currentPlayer]++;
            if (dst.city) { dst.player = _currentPlayer; }
            dst.unit    = UNIT_NONE;
            dst.player  = _currentPlayer;
            dst.unitHP  = 0;
        }
    } else {
        dst.unit     = src.unit;
        dst.player   = src.player;
        dst.unitHP   = src.unitHP;
        dst.unitMoved = true;
        src.unit     = UNIT_NONE;
        src.player   = (src.city ? src.player : 0);
        src.unitHP   = 0;
        if (dst.city && dst.player == 0) dst.player = _currentPlayer;
    }
    src.unitMoved = true;

    // Check win condition
    bool p1hasCities = false, p2hasCities = false;
    for (int r=0;r<MAP_H;r++) for (int q=0;q<MAP_W;q++) {
        if (_map[q][r].city) {
            if (_map[q][r].player==1) p1hasCities=true;
            if (_map[q][r].player==2) p2hasCities=true;
        }
    }
    if (!p1hasCities || !p2hasCities) _gameOver = true;
}

void StrategyApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { AppManager::instance().goHome(); return; }

    if (ev.type == TouchEvent::TAP) {
        // End turn button
        if (ev.x > EPD_W - 110 && ev.y > EPD_H - DOCK_H - 44) { endTurn(); return; }

        // Hex hit
        int q, r;
        if (pixelToHex(ev.x, ev.y, q, r)) selectHex(q, r);
    }
    if (ev.type == TouchEvent::MOVE) {
        _camX += ev.dx; _camY += ev.dy;
        draw();
    }
}
