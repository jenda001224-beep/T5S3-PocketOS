#include "notes.h"
#include "../ui/canvas.h"
#include "../hal/storage.h"
#include "../os/appmanager.h"

const char* NotesApp::KBD_ROWS[] = {
    "1234567890",
    "qwertyuiop",
    "asdfghjkl",
    "zxcvbnm.,!",
    " <Enter",
};

void NotesApp::onEnter() {
    _state = LIST;
    _count = Storage::instance().listDir(NOTES_PATH, _names, 20, ".txt");
    for (int i = 0; i < _count; i++) {
        String path = String(NOTES_PATH) + "/" + _names[i];
        _notes[i] = Storage::instance().readText(path.c_str());
    }
    _scroll = 0;
    draw();
}

void NotesApp::openNote(int idx) {
    _editIdx     = idx;
    _editContent = (idx < _count) ? _notes[idx] : "";
    _cursorPos   = _editContent.length();
    _state       = EDITING;
    _kbdVisible  = true;
    draw();
}

void NotesApp::newNote() {
    char name[32];
    snprintf(name, sizeof(name), "note%d.txt", _count + 1);
    _names[_count] = name;
    _notes[_count] = "";
    openNote(_count);
}

void NotesApp::saveNote() {
    if (_editIdx < 0) return;
    String path = String(NOTES_PATH) + "/" + _names[_editIdx];
    Storage::instance().writeText(path.c_str(), _editContent);
    _notes[_editIdx] = _editContent;
    if (_editIdx >= _count) _count = _editIdx + 1;
}

void NotesApp::drawList() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Notes");

    // New note button
    cv.fillRoundRect(EPD_W - 60, 8, 50, 28, 8, C_MID);
    cv.drawTextCentered(EPD_W - 35, 16, 50, "+", C_WHITE, C_MID, 2);

    int y = STATUS_BAR_H + 8;
    int lineH = 50;
    int maxVis = (EPD_H - y - DOCK_H) / lineH;

    if (_count == 0) {
        cv.drawTextCentered(EPD_W/2, EPD_H/2 - 8, EPD_W, "No notes yet. Tap + to create.", C_MID, C_WHITE, 1);
    }

    for (int i = _scroll; i < _count && i - _scroll < maxVis; i++) {
        bool odd = (i % 2) == 0;
        cv.fillRect(0, y, EPD_W, lineH-2, odd ? C_WHITE : 12);

        // Name
        String n = _names[i]; n.replace(".txt", "");
        cv.drawText(20, y + 6, n.c_str(), C_BLACK, odd ? C_WHITE : 12, 2);

        // Preview
        String preview = _notes[i].substring(0, 60);
        preview.replace("\n", " ");
        cv.drawText(20, y + 28, preview.c_str(), C_MID, odd ? C_WHITE : 12, 1);

        // Delete button
        cv.fillRoundRect(EPD_W - 60, y + 10, 50, 28, 6, C_LIGHT);
        cv.drawTextCentered(EPD_W - 35, y + 19, 50, "Del", C_DARK, C_LIGHT, 1);

        y += lineH;
    }
    cv.flushFull();
}

void NotesApp::drawEditor() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Edit Note");

    // Save button
    cv.fillRoundRect(EPD_W - 80, 8, 70, 28, 8, C_MID);
    cv.drawTextCentered(EPD_W - 45, 16, 70, "Save", C_WHITE, C_MID, 1);

    int kbdH   = _kbdVisible ? 200 : 0;
    int editH  = EPD_H - STATUS_BAR_H - kbdH - 8;
    int editY  = STATUS_BAR_H + 4;

    // Editor area
    cv.fillRect(0, editY, EPD_W, editH, 14);
    cv.drawRect(0, editY, EPD_W, editH, C_MID);

    // Word-wrap the content
    String content = _editContent + "|";  // cursor indicator
    int lineW = (EPD_W - 20) / 12;        // chars per line at scale 2
    int ly = editY + 6;
    int lineH = 18;
    int i = 0;
    while (i < (int)content.length() && ly < editY + editH - lineH) {
        String line = content.substring(i, i + lineW);
        int nl = line.indexOf('\n');
        if (nl >= 0) { line = line.substring(0, nl); i += nl + 1; }
        else          i += lineW;
        cv.drawText(10, ly, line.c_str(), C_BLACK, 14, 2);
        ly += lineH;
    }

    // Keyboard
    if (_kbdVisible) {
        int kbdY = EPD_H - kbdH;
        cv.fillRect(0, kbdY, EPD_W, kbdH, C_LIGHT);
        int nrows = 5;
        int rowH = kbdH / nrows;
        for (int row = 0; row < nrows; row++) {
            const char* keys = KBD_ROWS[row];
            int nkeys = strlen(keys);
            int keyW  = EPD_W / nkeys;
            for (int k = 0; k < nkeys; k++) {
                int kx = k * keyW;
                int ky = kbdY + row * rowH;
                cv.fillRoundRect(kx+1, ky+1, keyW-2, rowH-2, 3, C_WHITE);
                cv.drawRoundRect(kx+1, ky+1, keyW-2, rowH-2, 3, C_LIGHT);
                char ch[2] = { keys[k], 0 };
                cv.drawTextCentered(kx+keyW/2, ky+rowH/2-4, keyW, ch, C_BLACK, C_WHITE, 1);
            }
        }
    }

    cv.flushFull();
}

void NotesApp::draw() {
    if (_state == LIST) drawList();
    else                drawEditor();
}

void NotesApp::deleteNote(int idx) {
    String path = String(NOTES_PATH) + "/" + _names[idx];
    // Move remaining notes down
    for (int i = idx; i < _count - 1; i++) {
        _names[i] = _names[i+1];
        _notes[i] = _notes[i+1];
    }
    _count--;
    draw();
}

void NotesApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) {
        if (_state == EDITING) { saveNote(); _state = LIST; draw(); }
        else AppManager::instance().goHome();
        return;
    }

    if (_state == LIST) {
        if (ev.type == TouchEvent::TAP) {
            // New note button
            if (ev.x > EPD_W - 60 && ev.y < STATUS_BAR_H + 8) { newNote(); return; }

            int lineH  = 50;
            int y0     = STATUS_BAR_H + 8;
            int idx    = (ev.y - y0) / lineH + _scroll;

            if (idx >= 0 && idx < _count) {
                if (ev.x > EPD_W - 60) deleteNote(idx);
                else                    openNote(idx);
            }
        }
        if (ev.type == TouchEvent::SWIPE_UP)   { _scroll++; draw(); }
        if (ev.type == TouchEvent::SWIPE_DOWN)  { if (_scroll > 0) { _scroll--; draw(); } }
    } else {
        // Editor
        if (ev.type == TouchEvent::TAP) {
            // Save button
            if (ev.x > EPD_W - 80 && ev.y < STATUS_BAR_H + 8) { saveNote(); _state = LIST; draw(); return; }

            int kbdH = 200;
            int kbdY = EPD_H - kbdH;

            if (ev.y >= kbdY && _kbdVisible) {
                int nrows = 5;
                int rowH = kbdH / nrows;
                int row = (ev.y - kbdY) / rowH;
                if (row >= 0 && row < nrows) {
                    const char* keys = KBD_ROWS[row];
                    int nkeys = strlen(keys);
                    int keyW  = EPD_W / nkeys;
                    int col   = ev.x / keyW;
                    if (col >= 0 && col < nkeys) {
                        char k = keys[col];
                        if (k == '<') {
                            if (_editContent.length() > 0)
                                _editContent.remove(_editContent.length()-1);
                        } else if (k == 'E') {  // Enter
                            _editContent += '\n';
                        } else {
                            _editContent += k;
                        }
                        draw();
                    }
                }
            }
        }
    }
}
