#pragma once
#include "app.h"

#define SKETCH_APP_ID  12

// Finger drawing surface — a natural fit for e-ink. Strokes are drawn straight
// into the framebuffer; the affected region is pushed to the panel per stroke.
class SketchApp : public App {
public:
    SketchApp() : App("Sketch", SKETCH_APP_ID) {}
    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;

private:
    void drawToolbar();
    void clearCanvas();
    bool saveBMP();
    void flushStroke();

    int  _pen   = 4;       // stroke radius
    int  _minX, _minY, _maxX, _maxY;   // stroke bounding box
    int  _points = 0;
    char _status[40] = "";

    static constexpr int TOOLBAR_Y = STATUS_BAR_H;
    static constexpr int TOOLBAR_H = 40;
    static constexpr int DRAW_Y    = STATUS_BAR_H + 40;
};
