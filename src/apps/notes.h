#pragma once
#include "app.h"

#define NOTES_APP_ID  8

class NotesApp : public App {
public:
    NotesApp() : App("Notes", NOTES_APP_ID) {}
    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    enum State { LIST, EDITING };
    void drawList();
    void drawEditor();
    void openNote(int idx);
    void saveNote();
    void newNote();
    void deleteNote(int idx);

    State   _state = LIST;
    String  _notes[20];
    String  _names[20];
    int     _count      = 0;
    int     _editIdx    = -1;
    String  _editContent;
    int     _cursorPos  = 0;
    int     _scroll     = 0;
    bool    _kbdVisible = true;

    static const char* KBD_ROWS[];
};
