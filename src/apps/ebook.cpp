#include "ebook.h"
#include "../ui/canvas.h"
#include "../hal/storage.h"
#include "../os/appmanager.h"

// ─── Layout helpers ───────────────────────────────────────────────────────────

int EbookApp::charsPerLine() const { return READER_W / (6 * _fontScale); }
int EbookApp::linesPerPage() const { return READER_H / lineH(); }

// ─── onEnter / onLeave ────────────────────────────────────────────────────────

void EbookApp::onEnter() {
    _state       = LIST;
    _bookCount   = 0;
    _bookScroll  = 0;

    // Collect .txt books
    String names[32];
    int n = Storage::instance().listDir(BOOKS_PATH, names, 32, ".txt");
    for (int i = 0; i < n && _bookCount < 64; i++) {
        _names[_bookCount] = names[i];
        _names[_bookCount].replace(".txt", "");
        _paths[_bookCount] = String(BOOKS_PATH) + "/" + names[i];
        _types[_bookCount] = TYPE_TXT;
        _bookCount++;
    }

    // Collect .epub books
    n = Storage::instance().listDir(BOOKS_PATH, names, 32, ".epub");
    for (int i = 0; i < n && _bookCount < 64; i++) {
        _names[_bookCount] = names[i];
        _names[_bookCount].replace(".epub", "");
        _paths[_bookCount] = String(BOOKS_PATH) + "/" + names[i];
        _types[_bookCount] = TYPE_EPUB;
        _bookCount++;
    }

    draw();
}

void EbookApp::onLeave() {
    if (_file) _file.close();
    if (_epub.isOpen()) _epub.close();
}

// ─── Library list ─────────────────────────────────────────────────────────────

void EbookApp::drawLibrary() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Library");

    if (_bookCount == 0) {
        cv.drawTextCentered(EPD_W/2, EPD_H/2 - 20, 700,
            "No books found on SD card.", C_MID, C_WHITE, 2);
        cv.drawTextCentered(EPD_W/2, EPD_H/2 + 10, 700,
            "Copy .epub or .txt to /pocket/books/", C_MID, C_WHITE, 1);
        cv.flushFull();
        return;
    }

    int lineH2 = 52;
    int maxVis = (EPD_H - STATUS_BAR_H - 8 - DOCK_H) / lineH2;
    int y = STATUS_BAR_H + 8;

    for (int i = _bookScroll; i < _bookCount && i - _bookScroll < maxVis; i++) {
        bool odd = (i % 2) == 0;
        uint8_t bg = odd ? C_WHITE : 12;
        cv.fillRect(0, y, EPD_W, lineH2 - 2, bg);

        // Format badge
        bool isEpub = (_types[i] == TYPE_EPUB);
        cv.fillRoundRect(10, y + 14, isEpub ? 40 : 28, 18, 4,
            isEpub ? C_BLACK : C_MID);
        cv.drawTextCentered(isEpub ? 30 : 24, y + 20,
            isEpub ? 40 : 28,
            isEpub ? "EPUB" : "TXT",
            C_WHITE, isEpub ? C_BLACK : C_MID, 1);

        // Title
        cv.drawText(60, y + 8,  _names[i].c_str(), C_BLACK, bg, 2);

        // Arrow
        cv.drawText(EPD_W - 20, y + 22, ">", C_MID, bg, 1);
        y += lineH2;
    }

    // Scroll arrows
    if (_bookScroll > 0)
        cv.drawTextCentered(EPD_W/2, STATUS_BAR_H + 4, 100, "^ more", C_MID, C_WHITE, 1);
    if (_bookScroll + maxVis < _bookCount)
        cv.drawTextCentered(EPD_W/2, EPD_H - DOCK_H - 14, 100, "v more", C_MID, C_WHITE, 1);

    cv.flushFull();
}

// ─── EPUB chapter list ────────────────────────────────────────────────────────

void EbookApp::drawChapterList() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);

    // Nav bar shows book title
    drawNavBar(_epub.meta().title[0] ? _epub.meta().title : "Chapters");

    // Author subtitle
    if (_epub.meta().author[0]) {
        cv.fillRect(0, STATUS_BAR_H, EPD_W, 22, C_LIGHT);
        cv.drawTextCentered(EPD_W/2, STATUS_BAR_H + 7, EPD_W,
            _epub.meta().author, C_DARK, C_LIGHT, 1);
    }

    int startY = STATUS_BAR_H + 24;
    int lineH2 = 44;
    int maxVis = (EPD_H - startY - DOCK_H) / lineH2;
    int y      = startY;

    for (int i = _chapterScroll; i < _epub.chapterCount() && i - _chapterScroll < maxVis; i++) {
        bool odd = (i % 2) == 0;
        uint8_t bg = odd ? C_WHITE : 12;
        cv.fillRect(0, y, EPD_W, lineH2 - 2, bg);

        char num[6]; snprintf(num, sizeof(num), "%d.", i + 1);
        cv.drawText(10, y + 14, num, C_MID, bg, 1);
        cv.drawText(40, y + 10, _epub.chapter(i).title, C_BLACK, bg, 2);
        cv.drawText(EPD_W - 20, y + 18, ">", C_MID, bg, 1);
        y += lineH2;
    }

    if (_chapterScroll > 0)
        cv.drawTextCentered(EPD_W/2, startY + 2, 100, "^ more", C_MID, C_WHITE, 1);
    if (_chapterScroll + maxVis < _epub.chapterCount())
        cv.drawTextCentered(EPD_W/2, EPD_H - DOCK_H - 14, 100, "v more", C_MID, C_WHITE, 1);

    cv.flushFull();
}

// ─── Shared text reflow ───────────────────────────────────────────────────────

void EbookApp::reflowFrom(const String& text, int charOffset) {
    _lineCount = 0;
    int maxCPL = charsPerLine();
    int maxLPP = linesPerPage();
    int i      = charOffset;
    int len    = text.length();

    while (_lineCount < maxLPP && i < len) {
        // Find end of this line (hard newline or wrap at maxCPL chars)
        int lineEnd = i;
        int hardNL  = text.indexOf('\n', i);
        if (hardNL >= 0 && hardNL - i <= maxCPL) {
            lineEnd = hardNL;
        } else {
            lineEnd = i + maxCPL;
            if (lineEnd < len) {
                // Word-wrap: back up to last space
                int sp = lineEnd;
                while (sp > i && text[sp] != ' ') sp--;
                if (sp > i) lineEnd = sp;
            } else {
                lineEnd = len;
            }
        }

        int copyLen = min(lineEnd - i, 159);
        memcpy(_lines[_lineCount], text.c_str() + i, copyLen);
        _lines[_lineCount][copyLen] = '\0';
        _lineCount++;

        // Advance past newline or space
        i = lineEnd;
        if (i < len && (text[i] == '\n' || text[i] == ' ')) i++;
    }

    _nextCharOffset = i;
}

// ─── TXT reader ───────────────────────────────────────────────────────────────

void EbookApp::openTxt(const String& path) {
    if (_file) _file.close();
    _file      = Storage::instance().open(path.c_str());
    _fileSize  = _file ? _file.size() : 0;
    _pageStart = 0;
    _pageEnd   = 0;
    _type      = TYPE_TXT;
    _state     = READING;

    reflowTxt();
    draw();
}

void EbookApp::reflowTxt() {
    if (!_file) { _lineCount = 0; return; }
    _file.seek(_pageStart);

    int maxCPL = charsPerLine();
    int maxLPP = linesPerPage();
    _lineCount  = 0;

    char word[160];
    char line[160];
    int  lineLen = 0;
    word[0] = line[0] = '\0';

    while (_lineCount < maxLPP && _file.available()) {
        char c = _file.read();
        if (c == '\r') continue;

        bool flush = (c == ' ' || c == '\n');
        if (!flush) {
            int wl = strlen(word);
            if (wl < 159) { word[wl] = c; word[wl+1] = '\0'; }
            continue;
        }

        int wlen = strlen(word);
        int gap  = lineLen > 0 ? 1 : 0;
        if (lineLen + gap + wlen <= maxCPL) {
            if (lineLen > 0) { line[lineLen++] = ' '; }
            memcpy(line + lineLen, word, wlen + 1);
            lineLen += wlen;
        } else {
            strncpy(_lines[_lineCount++], line, 159);
            strncpy(line, word, 159);
            lineLen = wlen;
        }
        word[0] = '\0';

        if (c == '\n' && lineLen > 0 && _lineCount < maxLPP) {
            strncpy(_lines[_lineCount++], line, 159);
            line[0] = '\0'; lineLen = 0;
        }
    }
    if (lineLen > 0 && _lineCount < maxLPP)
        strncpy(_lines[_lineCount++], line, 159);

    _pageEnd = _file.position();
}

void EbookApp::nextPageTxt() {
    if (_pageEnd < (long)_fileSize) {
        _pageStart = _pageEnd;
        reflowTxt();
        draw();
    }
}

void EbookApp::prevPageTxt() {
    int lpp  = linesPerPage();
    int cpl  = charsPerLine();
    long delta = (long)lpp * cpl;
    _pageStart = max(0L, _pageStart - delta * 2);
    reflowTxt();
    draw();
}

// ─── EPUB reader ──────────────────────────────────────────────────────────────

void EbookApp::openEpub(const String& path) {
    if (_epub.isOpen()) _epub.close();
    if (!_epub.open(path.c_str())) {
        // Fallback: show error on screen
        Canvas& cv = Canvas::instance();
        cv.clear(C_WHITE);
        drawNavBar("Error");
        cv.drawTextCentered(EPD_W/2, EPD_H/2, EPD_W, "Failed to open EPUB", C_BLACK, C_WHITE, 2);
        cv.flushFull();
        return;
    }

    _type           = TYPE_EPUB;
    _epubChapter    = 0;
    _chapterScroll  = 0;
    _state          = CHAPTER_LIST;
    draw();
}

void EbookApp::loadEpubChapter(int chIdx) {
    _epubChapter    = chIdx;
    _pageCharOffset = 0;
    _pageText       = "";

    if (!_epub.loadChapter(chIdx, _pageText)) {
        _pageText = "Error loading chapter.";
    }

    reflowEpub();
    _state = READING;
    draw();
}

void EbookApp::reflowEpub() {
    reflowFrom(_pageText, _pageCharOffset);
}

void EbookApp::nextPageEpub() {
    if (_nextCharOffset < (int)_pageText.length()) {
        _pageCharOffset = _nextCharOffset;
        reflowEpub();
        draw();
    } else {
        // Next chapter
        if (_epubChapter + 1 < _epub.chapterCount()) {
            loadEpubChapter(_epubChapter + 1);
        }
    }
}

void EbookApp::prevPageEpub() {
    if (_pageCharOffset > 0) {
        // Jump back ~1 page worth of chars
        int delta = linesPerPage() * charsPerLine();
        _pageCharOffset = max(0, _pageCharOffset - delta * 2);
        reflowEpub();
        draw();
    } else if (_epubChapter > 0) {
        // Previous chapter, last page
        _epubChapter--;
        _pageText = "";
        _epub.loadChapter(_epubChapter, _pageText);
        // Jump to end
        _pageCharOffset = max(0, (int)_pageText.length() - linesPerPage() * charsPerLine());
        reflowEpub();
        _state = READING;
        draw();
    }
}

// ─── Open dispatcher ─────────────────────────────────────────────────────────

void EbookApp::openBook(int idx) {
    if (_types[idx] == TYPE_EPUB) openEpub(_paths[idx]);
    else                          openTxt (_paths[idx]);
}

// ─── Reader drawing ───────────────────────────────────────────────────────────

void EbookApp::drawReader() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);

    // Nav bar — show book title for EPUB
    if (_type == TYPE_EPUB && _epub.isOpen() && _epub.meta().title[0])
        drawNavBar(_epub.meta().title);
    else
        drawNavBar("Reading");

    // Text area
    int y = READER_Y;
    for (int i = 0; i < _lineCount; i++) {
        cv.drawText(READER_X, y, _lines[i], C_BLACK, C_WHITE, _fontScale);
        y += lineH();
    }

    // Progress bar
    int barY = EPD_H - DOCK_H - 16;
    cv.drawHLine(0, barY, EPD_W, C_LIGHT);

    float progress = 0;
    if (_type == TYPE_TXT && _fileSize > 0) {
        progress = (float)_pageStart / _fileSize;
    } else if (_type == TYPE_EPUB && _epub.chapterCount() > 0) {
        float chFrac = (float)_epubChapter / _epub.chapterCount();
        float pgFrac = (_pageText.length() > 0)
            ? (float)_pageCharOffset / _pageText.length() / _epub.chapterCount()
            : 0;
        progress = chFrac + pgFrac;
    }
    int fill = (int)(progress * EPD_W);
    cv.fillRect(0, barY, fill, 4, C_MID);

    // Chapter info for EPUB
    if (_type == TYPE_EPUB && _epub.isOpen()) {
        char info[64];
        snprintf(info, sizeof(info), "Ch %d/%d: %s",
            _epubChapter + 1, _epub.chapterCount(),
            _epub.chapter(_epubChapter).title);
        cv.drawText(10, barY + 6, info, C_MID, C_WHITE, 1);
    } else {
        char pct[8]; snprintf(pct, sizeof(pct), "%d%%", (int)(progress * 100));
        cv.drawText(10, barY + 6, pct, C_MID, C_WHITE, 1);
    }

    // Font scale buttons
    for (int s = 1; s <= 4; s++) {
        int bx = EPD_W - 96 + (s-1) * 22;
        bool sel = (s == _fontScale);
        cv.fillRoundRect(bx, barY + 4, 20, 18, 3, sel ? C_MID : C_LIGHT);
        char sc[2] = { (char)('0'+s), 0 };
        cv.drawTextCentered(bx+10, barY+8, 20, sc, C_BLACK, sel ? C_MID : C_LIGHT, 1);
    }

    // Chapter list button for EPUB
    if (_type == TYPE_EPUB) {
        cv.fillRoundRect(EPD_W - 120, barY - 36, 110, 26, 6, C_LIGHT);
        cv.drawTextCentered(EPD_W - 65, barY - 26, 110, "Chapters", C_BLACK, C_LIGHT, 1);
    }

    cv.flushFull();
}

// ─── draw ─────────────────────────────────────────────────────────────────────

void EbookApp::draw() {
    switch (_state) {
        case LIST:         drawLibrary();     break;
        case CHAPTER_LIST: drawChapterList(); break;
        case READING:      drawReader();      break;
    }
}

// ─── handleEvent ─────────────────────────────────────────────────────────────

void EbookApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) {
        switch (_state) {
            case READING:
                _state = (_type == TYPE_EPUB) ? CHAPTER_LIST : LIST;
                draw();
                break;
            case CHAPTER_LIST:
                _state = LIST;
                draw();
                break;
            case LIST:
                AppManager::instance().goHome();
                break;
        }
        return;
    }

    // ── Library list ──────────────────────────────────────────────────────────
    if (_state == LIST) {
        if (ev.type == TouchEvent::TAP) {
            int lineH2 = 52;
            int idx    = (ev.y - STATUS_BAR_H - 8) / lineH2 + _bookScroll;
            if (idx >= 0 && idx < _bookCount) openBook(idx);
        }
        if (ev.type == TouchEvent::SWIPE_UP)   { _bookScroll = min(_bookScroll+1, max(0,_bookCount-1)); draw(); }
        if (ev.type == TouchEvent::SWIPE_DOWN)  { if (_bookScroll > 0) { _bookScroll--; draw(); } }
        return;
    }

    // ── EPUB chapter list ─────────────────────────────────────────────────────
    if (_state == CHAPTER_LIST) {
        if (ev.type == TouchEvent::TAP) {
            int startY = STATUS_BAR_H + 24;
            int lineH2 = 44;
            int idx    = (ev.y - startY) / lineH2 + _chapterScroll;
            if (idx >= 0 && idx < _epub.chapterCount()) loadEpubChapter(idx);
        }
        if (ev.type == TouchEvent::SWIPE_UP)   { _chapterScroll++; draw(); }
        if (ev.type == TouchEvent::SWIPE_DOWN)  { if (_chapterScroll > 0) { _chapterScroll--; draw(); } }
        return;
    }

    // ── Reader ────────────────────────────────────────────────────────────────
    if (_state == READING) {
        // Font scale buttons
        if (ev.type == TouchEvent::TAP) {
            int barY = EPD_H - DOCK_H - 16;
            // Font size 1-4 buttons
            if (ev.y >= barY + 4 && ev.y < barY + 22 && ev.x > EPD_W - 96) {
                int s = (ev.x - (EPD_W - 96)) / 22 + 1;
                if (s >= 1 && s <= 4) { _fontScale = s; reflowTxt(); draw(); }
                return;
            }
            // "Chapters" button for EPUB
            if (_type == TYPE_EPUB && ev.y >= barY - 36 && ev.x > EPD_W - 120) {
                _state = CHAPTER_LIST;
                draw();
                return;
            }
            // Left/right half tap = prev/next page
            if (_type == TYPE_TXT) {
                if (ev.x > EPD_W / 2) nextPageTxt();
                else                   prevPageTxt();
            } else {
                if (ev.x > EPD_W / 2) nextPageEpub();
                else                   prevPageEpub();
            }
            return;
        }

        if (_type == TYPE_TXT) {
            if (ev.type == TouchEvent::SWIPE_LEFT)  nextPageTxt();
            if (ev.type == TouchEvent::SWIPE_RIGHT) prevPageTxt();
        } else {
            if (ev.type == TouchEvent::SWIPE_LEFT)  nextPageEpub();
            if (ev.type == TouchEvent::SWIPE_RIGHT) prevPageEpub();
        }
    }
}
