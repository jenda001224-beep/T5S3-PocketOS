#include "browser.h"
#include "../ui/canvas.h"
#include "../os/appmanager.h"
#include "../apps/settings.h"
#include <WiFiClientSecure.h>

const char* BrowserApp::KBD_ROWS[] = {
    "1234567890",
    "qwertyuiop",
    "asdfghjkl.",
    "zxcvbnm:/-",
    " <Go",
};

void BrowserApp::onEnter() {
    _state    = URL_INPUT;
    _scroll   = 0;
    draw();
}

String BrowserApp::stripHTML(const String& html) {
    String out;
    out.reserve(html.length());
    bool inTag  = false;
    bool inScript = false;
    for (int i = 0; i < (int)html.length(); i++) {
        char c = html[i];
        if (html.substring(i, i+7).equalsIgnoreCase("<script")) inScript = true;
        if (html.substring(i, i+9).equalsIgnoreCase("</script>")) { inScript = false; i += 8; continue; }
        if (inScript) continue;
        if (c == '<') { inTag = true; continue; }
        if (c == '>') { inTag = false; out += '\n'; continue; }
        if (inTag) continue;
        if (c == '&') {
            // Basic entity decode
            if (html.substring(i,i+4) == "&lt;")  { out += '<'; i+=3; }
            else if (html.substring(i,i+4) == "&gt;")  { out += '>'; i+=3; }
            else if (html.substring(i,i+5) == "&amp;") { out += '&'; i+=4; }
            else if (html.substring(i,i+6) == "&nbsp;") { out += ' '; i+=5; }
            else out += c;
            continue;
        }
        out += c;
    }
    // Collapse multiple blank lines
    String result;
    int blanks = 0;
    for (int i = 0; i < (int)out.length(); i++) {
        if (out[i] == '\n') { if (++blanks <= 2) result += '\n'; }
        else { blanks = 0; result += out[i]; }
    }
    return result;
}

void BrowserApp::navigate() {
    _state = LOADING;
    draw();

    SystemConfig& cfg = SettingsApp::config();
    if (!cfg.wifiEnabled || cfg.wifiSSID[0] == 0) {
        snprintf(_error, sizeof(_error), "Enable Wi-Fi in Settings first");
        _state = ERROR_STATE;
        draw();
        return;
    }

    // Make sure we are connected (connect on demand, wait up to ~8 s).
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(cfg.wifiSSID, cfg.wifiPass);
        uint32_t t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) delay(200);
    }
    if (WiFi.status() != WL_CONNECTED) {
        snprintf(_error, sizeof(_error), "Wi-Fi not connected");
        _state = ERROR_STATE;
        draw();
        return;
    }

    // Build a full URL (default to https which is what google.com needs).
    String url = _url;
    if (!url.startsWith("http://") && !url.startsWith("https://")) url = "https://" + url;

    WiFiClientSecure secure;
    secure.setInsecure();          // no cert store on device
    WiFiClient plain;

    HTTPClient http;
    bool https = url.startsWith("https://");
    if (https) http.begin(secure, url);
    else       http.begin(plain, url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(9000);
    http.addHeader("User-Agent", "Mozilla/5.0 (PocketOS)");
    http.addHeader("Accept", "text/html");
    int code = http.GET();

    if (code == 200) {
        String raw = http.getString();
        _content = stripHTML(raw);

        // Split into lines of ~80 chars
        _lineCount = 0;
        int start = 0;
        while (start < (int)_content.length() && _lineCount < 199) {
            int end = start + 80;
            if (end > (int)_content.length()) end = _content.length();
            int nl = _content.indexOf('\n', start);
            if (nl >= 0 && nl < end) end = nl;
            _lines[_lineCount++] = _content.substring(start, end);
            start = end + ((_content[end] == '\n') ? 1 : 0);
        }
        _scroll = 0;
        _state  = VIEWING;
    } else {
        snprintf(_error, sizeof(_error), "HTTP %d: %s", code, http.errorToString(code).c_str());
        _state = ERROR_STATE;
    }
    http.end();
    draw();
}

void BrowserApp::drawURLBar() {
    Canvas& cv = Canvas::instance();
    int barY = STATUS_BAR_H;
    cv.fillRect(0, barY, EPD_W, 34, C_LIGHT);
    cv.fillRoundRect(6, barY+4, EPD_W-56, 26, 6, C_WHITE);
    cv.drawRoundRect(6, barY+4, EPD_W-56, 26, 6, C_MID);

    char display[130];
    snprintf(display, sizeof(display), "%s|", _url);
    cv.drawText(12, barY + 11, display, C_BLACK, C_WHITE, 1);

    // Go button
    cv.fillRoundRect(EPD_W-46, barY+4, 40, 26, 6, C_MID);
    cv.drawTextCentered(EPD_W-26, barY+12, 40, "Go", C_WHITE, C_MID, 1);
}

void BrowserApp::drawContent() {
    Canvas& cv = Canvas::instance();
    int contentY = STATUS_BAR_H + 34;
    int contentH = EPD_H - contentY - DOCK_H;
    cv.fillRect(0, contentY, EPD_W, contentH, C_WHITE);

    int lineH  = 14;
    int maxVis = contentH / lineH;

    for (int i = _scroll; i < _lineCount && i - _scroll < maxVis; i++) {
        cv.drawText(10, contentY + (i - _scroll) * lineH, _lines[i].c_str(), C_BLACK, C_WHITE, 1);
    }

    // Scroll indicator
    if (_lineCount > maxVis) {
        int trackH = contentH - 10;
        int barH   = max(20, (int)((float)maxVis / _lineCount * trackH));
        int barY2  = contentY + 5 + (int)(((float)_scroll / (_lineCount - maxVis)) * (trackH - barH));
        cv.fillRect(EPD_W-6, contentY+5, 4, trackH, C_LIGHT);
        cv.fillRect(EPD_W-6, barY2, 4, barH, C_MID);
    }

    // Progress label
    char prog[16]; snprintf(prog, sizeof(prog), "%d/%d", _scroll+1, _lineCount);
    cv.drawText(10, EPD_H - DOCK_H - 16, prog, C_MID, C_WHITE, 1);
}

void BrowserApp::drawKeyboard() {
    Canvas& cv = Canvas::instance();
    int kbdY  = EPD_H / 2;
    int kbdH  = EPD_H - kbdY;
    cv.fillRect(0, kbdY, EPD_W, kbdH, C_LIGHT);

    int nrows = 5;
    int rowH  = kbdH / nrows;
    for (int row = 0; row < nrows; row++) {
        const char* keys = KBD_ROWS[row];
        int nkeys = strlen(keys);
        int keyW  = EPD_W / nkeys;
        for (int k = 0; k < nkeys; k++) {
            int kx = k * keyW;
            int ky = kbdY + row * rowH;
            cv.fillRoundRect(kx+1, ky+1, keyW-2, rowH-2, 3, C_WHITE);
            char ch[2] = {keys[k], 0};
            cv.drawTextCentered(kx+keyW/2, ky+rowH/2-4, keyW, ch, C_BLACK, C_WHITE, 1);
        }
    }
}

void BrowserApp::draw() {
    Canvas& cv = Canvas::instance();
    cv.clear(C_WHITE);
    drawNavBar("Browser");

    if (_state == URL_INPUT) {
        drawURLBar();
        drawKeyboard();
    } else if (_state == LOADING) {
        drawURLBar();
        cv.drawTextCentered(EPD_W/2, EPD_H/2-8, EPD_W, "Loading...", C_MID, C_WHITE, 2);
    } else if (_state == VIEWING) {
        drawURLBar();
        drawContent();
    } else if (_state == ERROR_STATE) {
        drawURLBar();
        cv.drawTextCentered(EPD_W/2, EPD_H/2-16, EPD_W, "Error:", C_BLACK, C_WHITE, 2);
        cv.drawTextCentered(EPD_W/2, EPD_H/2+10,  EPD_W, _error, C_MID, C_WHITE, 1);
    }

    cv.flushFull();
}

void BrowserApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { AppManager::instance().goHome(); return; }

    if (ev.type == TouchEvent::TAP) {
        int barY = STATUS_BAR_H;
        // Go button
        if (ev.x > EPD_W-46 && ev.y >= barY && ev.y < barY+34) { navigate(); return; }
        // URL bar — focus
        if (ev.y >= barY && ev.y < barY+34) { _state = URL_INPUT; draw(); return; }

        if (_state == URL_INPUT) {
            // Keyboard
            int kbdY  = EPD_H / 2;
            int nrows = 5;
            int rowH  = (EPD_H - kbdY) / nrows;
            int row   = (ev.y - kbdY) / rowH;
            if (row >= 0 && row < nrows) {
                const char* keys = KBD_ROWS[row];
                int nkeys = strlen(keys);
                int keyW  = EPD_W / nkeys;
                int col   = ev.x / keyW;
                if (col >= 0 && col < nkeys) {
                    char k = keys[col];
                    if (k == '<') { if (_urlLen > 0) _url[--_urlLen] = '\0'; }
                    else if (k == 'G' && row == 4) { navigate(); return; }
                    else if (_urlLen < 127) { _url[_urlLen++] = k; _url[_urlLen] = '\0'; }
                    draw();
                }
            }
        }
    }

    if (_state == VIEWING) {
        if (ev.type == TouchEvent::SWIPE_UP)   { _scroll = min(_scroll + 3, max(0, _lineCount-1)); draw(); }
        if (ev.type == TouchEvent::SWIPE_DOWN)  { _scroll = max(0, _scroll - 3);                   draw(); }
    }
}
