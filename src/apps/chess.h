#pragma once
#include "app.h"
#include <stdint.h>

#define CHESS_APP_ID  2

// Piece codes: 0=empty, 1-6=white pawn/knight/bishop/rook/queen/king
//              9-14=black pawn/knight/bishop/rook/queen/king
enum Piece : uint8_t {
    EMPTY=0,
    W_PAWN=1, W_KNIGHT=2, W_BISHOP=3, W_ROOK=4, W_QUEEN=5, W_KING=6,
    B_PAWN=9, B_KNIGHT=10, B_BISHOP=11, B_ROOK=12, B_QUEEN=13, B_KING=14,
};

struct Move { int8_t fr, fc, tr, tc; };

class ChessApp : public App {
public:
    ChessApp() : App("Chess", CHESS_APP_ID) {}

    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    void newGame();
    void drawBoard();
    void drawPiece(int r, int c, Piece p);
    void selectSquare(int r, int c);
    bool isLegalMove(int fr, int fc, int tr, int tc);
    bool inCheck(bool white);
    void applyMove(const Move& m);
    Move bestMove(int depth);         // simple minimax
    int  evaluate();

    // Generate pseudo-legal moves for piece at (r,c)
    int  genMoves(int r, int c, Move* out, int maxOut);

    Piece  _board[8][8];
    bool   _whiteTurn = true;
    bool   _selected  = false;
    int8_t _selR = -1, _selC = -1;
    bool   _gameOver  = false;
    char   _statusMsg[64] = "";

    static constexpr int BOARD_X   = (EPD_W - 8 * 60) / 2;
    static constexpr int BOARD_Y   = STATUS_BAR_H + 10;
    static constexpr int SQ        = 58;
};
