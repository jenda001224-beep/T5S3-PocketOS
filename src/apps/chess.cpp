#include "chess.h"
#include "../ui/canvas.h"
#include "../os/appmanager.h"

static const char PIECE_CHAR[] = " PNBRQK???pnbrqk";

void ChessApp::onEnter() {
    newGame();
    draw();
}

void ChessApp::newGame() {
    memset(_board, 0, sizeof(_board));
    // White pieces
    const Piece backRank[] = { W_ROOK,W_KNIGHT,W_BISHOP,W_QUEEN,W_KING,W_BISHOP,W_KNIGHT,W_ROOK };
    for (int c = 0; c < 8; c++) {
        _board[7][c] = backRank[c];
        _board[6][c] = W_PAWN;
        _board[1][c] = B_PAWN;
        _board[0][c] = (Piece)(backRank[c] + 8);  // black = +8
    }
    _whiteTurn = true;
    _selected  = false;
    _gameOver  = false;
    snprintf(_statusMsg, sizeof(_statusMsg), "White to move");
}

static bool isWhite(Piece p) { return p >= W_PAWN && p <= W_KING; }
static bool isBlack(Piece p) { return p >= B_PAWN && p <= B_KING; }

void ChessApp::drawBoard() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Chess");

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int x = BOARD_X + c * SQ;
            int y = BOARD_Y + r * SQ;
            bool light = (r + c) % 2 == 0;
            uint8_t sq_col = light ? C_WHITE : C_LIGHT;

            // Highlight selected
            if (_selected && _selR == r && _selC == c) sq_col = C_MID;

            cv.fillRect(x, y, SQ, SQ, sq_col);
            cv.drawRect(x, y, SQ, SQ, C_DARK);

            drawPiece(r, c, _board[r][c]);
        }
    }

    // Rank / file labels
    for (int i = 0; i < 8; i++) {
        char lbl[2] = { (char)('a' + i), 0 };
        cv.drawText(BOARD_X + i * SQ + SQ/2 - 3, BOARD_Y + 8*SQ + 4, lbl, C_DARK, C_WHITE, 1);
        char rlbl[2] = { (char)('8' - i), 0 };
        cv.drawText(BOARD_X - 12, BOARD_Y + i * SQ + SQ/2 - 4, rlbl, C_DARK, C_WHITE, 1);
    }

    // Status
    cv.drawTextCentered(EPD_W/2, BOARD_Y + 8*SQ + 20, EPD_W, _statusMsg, C_BLACK, C_WHITE, 1);

    // New game button
    cv.fillRoundRect(EPD_W - 110, BOARD_Y + 8*SQ + 10, 100, 28, 8, C_MID);
    cv.drawTextCentered(EPD_W - 60, BOARD_Y + 8*SQ + 18, 100, "New Game", C_BLACK, C_MID, 1);

    cv.flushFull();
}

void ChessApp::drawPiece(int r, int c, Piece p) {
    if (p == EMPTY) return;
    Canvas& cv = Canvas::instance();
    int x  = BOARD_X + c * SQ + SQ/2;
    int y  = BOARD_Y + r * SQ + SQ/2;
    bool white = isWhite(p);
    uint8_t fg = white ? C_BLACK : C_BLACK;
    uint8_t bg = white ? C_WHITE : C_DARK;

    cv.fillCircle(x, y, SQ/2 - 6, bg);
    cv.drawCircle(x, y, SQ/2 - 6, C_BLACK);

    char sym[2] = { PIECE_CHAR[p], 0 };
    cv.drawTextCentered(x, y - 4, SQ, sym, white ? C_BLACK : C_WHITE, bg, 2);
}

void ChessApp::draw() { drawBoard(); }

void ChessApp::selectSquare(int r, int c) {
    if (!_selected) {
        Piece p = _board[r][c];
        if (p == EMPTY) return;
        if (_whiteTurn && !isWhite(p)) return;
        if (!_whiteTurn && !isBlack(p)) return;
        _selected = true; _selR = r; _selC = c;
    } else {
        if (r == _selR && c == _selC) { _selected = false; draw(); return; }
        if (isLegalMove(_selR, _selC, r, c)) {
            Move m{_selR, _selC, (int8_t)r, (int8_t)c};
            applyMove(m);
            _selected   = false;
            _whiteTurn  = !_whiteTurn;

            if (inCheck(!_whiteTurn)) {
                snprintf(_statusMsg, sizeof(_statusMsg), "%s in check!", _whiteTurn ? "Black" : "White");
            } else {
                snprintf(_statusMsg, sizeof(_statusMsg), "%s to move", _whiteTurn ? "White" : "Black");
            }
            draw();

            // Simple AI for black
            if (!_whiteTurn && !_gameOver) {
                delay(200);
                Move ai = bestMove(2);
                if (ai.fr >= 0) {
                    applyMove(ai);
                    _whiteTurn = true;
                    snprintf(_statusMsg, sizeof(_statusMsg), "White to move");
                    draw();
                }
            }
        } else {
            _selected = false;
        }
    }
    draw();
}

bool ChessApp::isLegalMove(int fr, int fc, int tr, int tc) {
    if (tr < 0 || tr > 7 || tc < 0 || tc > 7) return false;
    Piece src = _board[fr][fc];
    Piece dst = _board[tr][tc];
    if (isWhite(src) && isWhite(dst)) return false;
    if (isBlack(src) && isBlack(dst)) return false;

    int dr = tr - fr, dc = tc - fc;

    switch (src) {
        case W_PAWN:
            if (dc == 0 && dr == -1 && dst == EMPTY) return true;
            if (dc == 0 && dr == -2 && fr == 6 && _board[fr-1][fc] == EMPTY && dst == EMPTY) return true;
            if (abs(dc) == 1 && dr == -1 && isBlack(dst)) return true;
            break;
        case B_PAWN:
            if (dc == 0 && dr == 1 && dst == EMPTY) return true;
            if (dc == 0 && dr == 2 && fr == 1 && _board[fr+1][fc] == EMPTY && dst == EMPTY) return true;
            if (abs(dc) == 1 && dr == 1 && isWhite(dst)) return true;
            break;
        case W_KNIGHT: case B_KNIGHT:
            return (abs(dr)==2 && abs(dc)==1) || (abs(dr)==1 && abs(dc)==2);
        case W_BISHOP: case B_BISHOP:
            if (abs(dr) != abs(dc)) return false;
            { int sr = dr>0?1:-1, sc = dc>0?1:-1;
              for (int i=1;i<abs(dr);i++) if (_board[fr+i*sr][fc+i*sc]!=EMPTY) return false; }
            return true;
        case W_ROOK: case B_ROOK:
            if (dr != 0 && dc != 0) return false;
            { int sr=dr==0?0:(dr>0?1:-1), sc=dc==0?0:(dc>0?1:-1);
              int steps=max(abs(dr),abs(dc));
              for (int i=1;i<steps;i++) if (_board[fr+i*sr][fc+i*sc]!=EMPTY) return false; }
            return true;
        case W_QUEEN: case B_QUEEN: {
            bool diag = abs(dr)==abs(dc), straight = dr==0||dc==0;
            if (!diag && !straight) return false;
            int sr=dr==0?0:(dr>0?1:-1), sc=dc==0?0:(dc>0?1:-1);
            int steps=max(abs(dr),abs(dc));
            for (int i=1;i<steps;i++) if (_board[fr+i*sr][fc+i*sc]!=EMPTY) return false;
            return true;
        }
        case W_KING: case B_KING:
            return abs(dr)<=1 && abs(dc)<=1;
        default: break;
    }
    return false;
}

bool ChessApp::inCheck(bool white) {
    // Find king
    Piece king = white ? W_KING : B_KING;
    int kr=-1, kc=-1;
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) if (_board[r][c]==king){kr=r;kc=c;}
    if (kr<0) return false;
    // Check if any enemy piece attacks king
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) {
        Piece p = _board[r][c];
        if (p==EMPTY) continue;
        if (white && isWhite(p)) continue;
        if (!white && isBlack(p)) continue;
        if (isLegalMove(r,c,kr,kc)) return true;
    }
    return false;
}

void ChessApp::applyMove(const Move& m) {
    Piece p = _board[m.fr][m.fc];
    // Pawn promotion
    if (p == W_PAWN && m.tr == 0) p = W_QUEEN;
    if (p == B_PAWN && m.tr == 7) p = B_QUEEN;
    _board[m.tr][m.tc] = p;
    _board[m.fr][m.fc] = EMPTY;
}

int ChessApp::evaluate() {
    static const int vals[] = {0,1,3,3,5,9,100, 0,0,-1,-3,-3,-5,-9,-100};
    int score = 0;
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) {
        Piece p = _board[r][c];
        if (p < 16) score += vals[p];
    }
    return score;
}

Move ChessApp::bestMove(int depth) {
    Move best{-1,-1,-1,-1};
    int bestScore = 10000;  // AI is black, minimizing
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) {
        Piece p = _board[r][c];
        if (!isBlack(p)) continue;
        for (int tr=0;tr<8;tr++) for (int tc=0;tc<8;tc++) {
            if (!isLegalMove(r,c,tr,tc)) continue;
            Piece saved = _board[tr][tc];
            Move m{(int8_t)r,(int8_t)c,(int8_t)tr,(int8_t)tc};
            applyMove(m);
            int score = evaluate();
            _board[r][c]   = p;
            _board[tr][tc] = saved;
            if (score < bestScore) { bestScore = score; best = m; }
        }
    }
    return best;
}

void ChessApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { AppManager::instance().goHome(); return; }

    // New game button
    if (ev.type == TouchEvent::TAP &&
        ev.y > BOARD_Y + 8*SQ + 10 && ev.x > EPD_W - 110) {
        newGame(); draw(); return;
    }

    if (ev.type == TouchEvent::TAP && !_gameOver && _whiteTurn) {
        int c = (ev.x - BOARD_X) / SQ;
        int r = (ev.y - BOARD_Y) / SQ;
        if (r >= 0 && r < 8 && c >= 0 && c < 8)
            selectSquare(r, c);
    }
}
