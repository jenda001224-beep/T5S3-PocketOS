#pragma once
#include "../hal/touch.h"

// Every app derives from this. The OS calls these methods.
class App {
public:
    virtual ~App() = default;

    virtual void onEnter()  {}                      // called when app launches
    virtual void onLeave()  {}                      // called when app is hidden
    virtual void onResume() { onEnter(); }          // after returning from another app
    virtual void draw()     = 0;                    // full redraw
    virtual void handleEvent(const TouchEvent& ev) {}
    virtual void tick() {}                          // called ~1/s for clock etc.

    const char* name() const { return _name; }
    uint8_t     id()   const { return _id; }

protected:
    App(const char* name, uint8_t id) : _name(name), _id(id) {}
    const char* _name;
    uint8_t     _id;

    // Convenience: draw a top navigation bar and return button
    void drawNavBar(const char* title);
    // Checks if tap is inside back button (top-left corner)
    bool isBackTap(const TouchEvent& ev);
};
