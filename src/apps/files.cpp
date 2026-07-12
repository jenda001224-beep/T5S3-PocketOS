#include "files.h"
#include "../ui/canvas.h"
#include "../hal/storage.h"
#include "../os/appmanager.h"
#include <SD.h>

void FilesApp::onEnter() {
    _path   = "/pocket";
    if (!SD.exists(_path)) _path = "/";
    _state  = LIST;
    _scroll = 0;
    loadDir();
    draw();
}

String FilesApp::fullPath(const String& name) {
    if (_path == "/") return "/" + name;
    return _path + "/" + name;
}

bool FilesApp::isTextName(const String& n) {
    String l = n; l.toLowerCase();
    return l.endsWith(".txt") || l.endsWith(".json") || l.endsWith(".csv") ||
           l.endsWith(".md")  || l.endsWith(".log")  || l.endsWith(".ini") ||
           l.endsWith(".cfg");
}

void FilesApp::loadDir() {
    _count = 0;
    File dir = SD.open(_path);
    if (!dir || !dir.isDirectory()) return;

    // directories first, then files
    for (int pass = 0; pass < 2; pass++) {
        dir.rewindDirectory();
        while (_count < MAX_ENTRIES) {
            File f = dir.openNextFile();
            if (!f) break;
            bool isDir = f.isDirectory();
            String nm = f.name();
            int slash = nm.lastIndexOf('/');
            if (slash >= 0) nm = nm.substring(slash + 1);
            uint32_t sz = f.size();
            f.close();
            if ((pass == 0) == isDir) {
                _ent[_count].name = nm;
                _ent[_count].dir  = isDir;
                _ent[_count].size = sz;
                _count++;
            }
        }
    }
    dir.close();
}

void FilesApp::enter(int idx) {
    if (idx < 0 || idx >= _count) return;
    if (_ent[idx].dir) {
        _path = fullPath(_ent[idx].name);
        _scroll = 0;
        loadDir();
        draw();
    } else {
        _sel = idx;
        if (isTextName(_ent[idx].name)) openText(fullPath(_ent[idx].name));
        else { _state = CONFIRM; draw(); }
    }
}

void FilesApp::openText(const String& full) {
    File f = SD.open(full);
    if (!f) return;
    _view = "";
    size_t limit = 12000;                 // cap preview
    while (f.available() && _view.length() < limit) _view += (char)f.read();
    if (f.available()) _view += "\n... [truncated]";
    f.close();
    _vScroll = 0;
    _state = VIEW;
    draw();
}

static String humanSize(uint32_t b) {
    if (b < 1024) return String(b) + " B";
    if (b < 1024*1024) return String(b/1024.0f, 1) + " KB";
    return String(b/(1024.0f*1024.0f), 1) + " MB";
}

void FilesApp::drawList() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Files");

    // path bar
    cv.fillRect(0, STATUS_BAR_H, EPD_W, 24, C_LIGHT);
    cv.drawText(10, STATUS_BAR_H + 8, _path.c_str(), C_BLACK, C_LIGHT, 1);

    int y = STATUS_BAR_H + 28;
    int lineH = 34;
    int maxVis = (EPD_H - y - DOCK_H) / lineH;

    // ".." row when not at root
    int shown = 0;
    if (_scroll == 0 && _path != "/") {
        cv.fillRect(0, y, EPD_W, lineH-2, C_WHITE);
        cv.drawText(20, y+8, "[..]", C_DARK, C_WHITE, 2);
        y += lineH; shown++;
    }

    for (int i = _scroll; i < _count && shown < maxVis; i++, shown++) {
        bool odd = (i % 2) == 0;
        uint8_t bg = odd ? C_WHITE : 12;
        cv.fillRect(0, y, EPD_W, lineH-2, bg);
        String label = (_ent[i].dir ? "[" + _ent[i].name + "]" : _ent[i].name);
        cv.drawText(20, y+8, label.c_str(), C_BLACK, bg, 2);
        if (!_ent[i].dir)
            cv.drawText(EPD_W-140, y+10, humanSize(_ent[i].size).c_str(), C_MID, bg, 1);
        y += lineH;
    }

    char cnt[40]; snprintf(cnt, sizeof(cnt), "%d items", _count);
    cv.drawText(10, EPD_H - DOCK_H - 14, cnt, C_MID, C_WHITE, 1);
    cv.flushFull();
}

void FilesApp::drawView() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Preview");

    int y = STATUS_BAR_H + 8;
    int lineH = 16;
    int maxVis = (EPD_H - y - DOCK_H) / lineH;
    int cpl = (EPD_W - 20) / 6;   // chars per line at scale 1

    // naive wrap + scroll over the preview text
    int line = 0, drawn = 0, col = 0, startLine = _vScroll;
    String cur = "";
    for (int i = 0; i < (int)_view.length(); i++) {
        char c = _view[i];
        if (c == '\r') continue;
        if (c == '\n' || col >= cpl) {
            if (line >= startLine && drawn < maxVis) {
                cv.drawText(10, y + drawn*lineH, cur.c_str(), C_BLACK, C_WHITE, 1);
                drawn++;
            }
            line++; col = 0; cur = "";
            if (c == '\n') continue;
        }
        cur += c; col++;
    }
    if (line >= startLine && drawn < maxVis && cur.length())
        cv.drawText(10, y + drawn*lineH, cur.c_str(), C_BLACK, C_WHITE, 1);

    cv.drawText(10, EPD_H - DOCK_H - 14, "swipe up/down · Home to close", C_MID, C_WHITE, 1);
    cv.flushFull();
}

void FilesApp::drawConfirm() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("File");

    cv.drawTextCentered(EPD_W/2, 120, EPD_W, _ent[_sel].name.c_str(), C_BLACK, C_WHITE, 2);
    cv.drawTextCentered(EPD_W/2, 150, EPD_W, humanSize(_ent[_sel].size).c_str(), C_MID, C_WHITE, 1);

    cv.fillRoundRect(EPD_W/2 - 220, 220, 200, 60, 10, C_DARK);
    cv.drawTextCentered(EPD_W/2 - 120, 240, 200, "Delete", C_WHITE, C_DARK, 2);
    cv.fillRoundRect(EPD_W/2 + 20, 220, 200, 60, 10, C_LIGHT);
    cv.drawTextCentered(EPD_W/2 + 120, 240, 200, "Cancel", C_BLACK, C_LIGHT, 2);

    cv.flushFull();
}

void FilesApp::draw() {
    switch (_state) {
        case LIST:    drawList();    break;
        case VIEW:    drawView();    break;
        case CONFIRM: drawConfirm(); break;
    }
}

void FilesApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) {
        if (_state != LIST) { _state = LIST; draw(); }
        else if (_path != "/pocket" && _path != "/") {
            int s = _path.lastIndexOf('/');
            _path = (s <= 0) ? "/" : _path.substring(0, s);
            _scroll = 0; loadDir(); draw();
        } else AppManager::instance().goHome();
        return;
    }

    if (_state == VIEW) {
        if (ev.type == TouchEvent::SWIPE_UP)   { _vScroll += 12; draw(); }
        if (ev.type == TouchEvent::SWIPE_DOWN) { _vScroll = max(0, _vScroll - 12); draw(); }
        return;
    }

    if (_state == CONFIRM) {
        if (ev.type != TouchEvent::TAP) return;
        if (ev.y >= 220 && ev.y < 280) {
            if (ev.x < EPD_W/2) {                 // Delete
                SD.remove(fullPath(_ent[_sel].name).c_str());
                _state = LIST; loadDir();
            } else _state = LIST;                 // Cancel
            draw();
        }
        return;
    }

    // LIST
    int y0 = STATUS_BAR_H + 28;
    int lineH = 34;
    if (ev.type == TouchEvent::TAP) {
        int row = (ev.y - y0) / lineH;
        if (_scroll == 0 && _path != "/") {
            if (row == 0) {                        // ".."
                int s = _path.lastIndexOf('/');
                _path = (s <= 0) ? "/" : _path.substring(0, s);
                _scroll = 0; loadDir(); draw(); return;
            }
            row -= 1;
        }
        enter(_scroll + row);
        return;
    }
    if (ev.type == TouchEvent::SWIPE_UP)   { if (_scroll + 6 < _count) { _scroll += 6; draw(); } }
    if (ev.type == TouchEvent::SWIPE_DOWN) { _scroll = max(0, _scroll - 6); draw(); }
}
