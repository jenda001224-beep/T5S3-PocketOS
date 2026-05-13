#pragma once
#include "app.h"

#define MAPS_APP_ID  3

// Offline tile-based maps.
// Tiles stored on SD: /pocket/maps/{zoom}/{x}/{y}.raw
// Each tile: 256×256 pixels, 4bpp grayscale (32KB raw)
class MapsApp : public App {
public:
    MapsApp() : App("Maps", MAPS_APP_ID) {}

    void onEnter() override;
    void draw()    override;
    void handleEvent(const TouchEvent& ev) override;
    void tick()    override;

private:
    void drawMap();
    void drawCompass();
    void drawGPSInfo();
    void drawTile(int tx, int ty, int px, int py, int zoom);
    void loadTileRaw(int tx, int ty, int zoom, uint8_t* buf);
    void latLngToTile(double lat, double lng, int zoom, int& tx, int& ty);
    void moveMap(int dx, int dy);
    void setZoom(int z);

    double  _centerLat  =  50.075;  // Prague default
    double  _centerLng  =  14.437;
    int     _zoom       = 13;
    int     _panX       = 0;  // pixel offset within tile
    int     _panY       = 0;
    bool    _followGPS  = true;

    static constexpr int TILE_PX = 256;
    static constexpr int MAP_Y   = STATUS_BAR_H;
    static constexpr int MAP_H   = EPD_H - MAP_Y - DOCK_H;
};
