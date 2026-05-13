#pragma once
#include "../config.h"
#include "../hal/touch.h"

struct AppEntry {
    const char*     name;
    const uint8_t*  icon;   // 16×16 bitmap
    uint8_t         appId;
};

class HomeScreen {
public:
    static HomeScreen& instance();

    void   begin(const AppEntry* apps, int count);
    void   draw();
    int    handleEvent(const TouchEvent& ev);   // returns appId to launch, or -1

    void   setPage(int page);
    int    page() const { return _page; }
    int    totalPages() const;

private:
    HomeScreen() = default;
    void   drawDock();
    void   drawPageDots();
    void   drawAppIcon(int slot, const AppEntry& app, bool highlight);
    int    hitTest(int x, int y);    // returns slot index in current page, or -1

    const AppEntry* _apps    = nullptr;
    int             _count   = 0;
    int             _page    = 0;
    int             _highlighted = -1;

public:
    // Dock: fixed 4 apps always visible (first 4 entries)
    static constexpr int DOCK_COUNT = 4;
private:
    static constexpr int GRID_COUNT = APP_COLS * APP_ROWS;

    int    appsPerPage() const { return GRID_COUNT; }
    int    pageOffset()  const { return _page * GRID_COUNT + DOCK_COUNT; }
};
