#pragma once
#include "app.h"
#include <WiFi.h>
#include <HTTPClient.h>

#define BROWSER_APP_ID  10

// Minimal text-mode browser — fetches a URL, strips HTML tags,
// renders plain text with scrolling. No JS, no CSS.
class BrowserApp : public App {
public:
    BrowserApp() : App("Browser", BROWSER_APP_ID) {}
    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    enum State { URL_INPUT, LOADING, VIEWING, ERROR_STATE };
    void drawURLBar();
    void drawContent();
    void drawKeyboard();
    void navigate();
    String stripHTML(const String& html);

    State   _state    = URL_INPUT;
    char    _url[128] = "http://lite.cnn.com";
    int     _urlLen   = 20;
    String  _content;
    String  _lines[200];
    int     _lineCount = 0;
    int     _scroll    = 0;
    char    _error[64] = "";

    static const char* KBD_ROWS[];
};
