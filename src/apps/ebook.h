#pragma once
#include "app.h"
#include <SD.h>
#include "../epub/epub_parser.h"

#define EBOOK_APP_ID   1

class EbookApp : public App {
public:
    EbookApp() : App("Books", EBOOK_APP_ID) {}

    void onEnter()  override;
    void onLeave()  override;
    void draw()     override;
    void handleEvent(const TouchEvent& ev) override;

private:
    enum State    { LIST, CHAPTER_LIST, READING };
    enum BookType { TYPE_TXT, TYPE_EPUB };

    // ── Library ───────────────────────────────────────────────────────────────
    void drawLibrary();
    void drawChapterList();  // EPUB chapter selection screen
    void drawReader();

    void openBook(int idx);

    // ── TXT reader ────────────────────────────────────────────────────────────
    void openTxt(const String& path);
    void reflowTxt();
    void nextPageTxt();
    void prevPageTxt();

    // ── EPUB reader ───────────────────────────────────────────────────────────
    void openEpub(const String& path);
    void loadEpubChapter(int chIdx);
    void reflowEpub();
    void nextPageEpub();
    void prevPageEpub();

    // ── Shared reflow ─────────────────────────────────────────────────────────
    // Fills _lines[] from _pageText starting at _pageCharOffset.
    // Sets _nextCharOffset to where the next page begins.
    void reflowFrom(const String& text, int charOffset);

    State    _state       = LIST;
    BookType _type        = TYPE_TXT;

    // Library
    String   _names[64];  // display names
    String   _paths[64];  // full SD paths
    BookType _types[64];
    int      _bookCount   = 0;
    int      _bookScroll  = 0;

    // TXT reader
    File     _file;
    size_t   _fileSize    = 0;
    long     _pageStart   = 0;
    long     _pageEnd     = 0;

    // EPUB reader
    EpubParser _epub;
    int      _epubChapter    = 0;   // current chapter index
    String   _pageText;             // full text of current EPUB chapter
    int      _pageCharOffset = 0;   // char offset into _pageText for current page
    int      _nextCharOffset = 0;   // first char of next page

    // Chapter list scroll
    int      _chapterScroll  = 0;

    // Shared
    int      _fontScale   = 2;      // 1-4 (maps to canvas scale param)
    static constexpr int MAX_LINES = 48;
    char     _lines[MAX_LINES][160];
    int      _lineCount   = 0;

    // Layout helpers
    int  charsPerLine() const;
    int  linesPerPage() const;
    int  lineH()        const { return 8 * _fontScale + 4; }
    static constexpr int READER_X = 36;
    static constexpr int READER_Y = STATUS_BAR_H + 16;
    static constexpr int READER_W = EPD_W - 72;
    static constexpr int READER_H = EPD_H - READER_Y - DOCK_H - 24;
};
