#pragma once
#include "app.h"
#include <Arduino.h>

#define FILES_APP_ID  14

// Simple SD-card file manager: browse folders, preview text files, delete.
class FilesApp : public App {
public:
    FilesApp() : App("Files", FILES_APP_ID) {}
    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    enum State { LIST, VIEW, CONFIRM };
    static constexpr int MAX_ENTRIES = 96;

    struct Entry { String name; bool dir; uint32_t size; };

    State   _state = LIST;
    String  _path  = "/pocket";
    Entry   _ent[MAX_ENTRIES];
    int     _count  = 0;
    int     _scroll = 0;
    int     _sel    = -1;

    // text viewer
    String  _view;
    int     _vScroll = 0;

    void   loadDir();
    void   enter(int idx);
    void   openText(const String& full);
    bool   isTextName(const String& n);
    String fullPath(const String& name);
    void   drawList();
    void   drawView();
    void   drawConfirm();
};
