#include "maps.h"
#include "../ui/canvas.h"
#include "../hal/gps_hw.h"
#include "../hal/storage.h"
#include "../os/appmanager.h"
#include <math.h>

#undef PI  // Arduino.h defines PI as a macro; undefine before constexpr
static constexpr double PI = 3.14159265358979323846;

void MapsApp::onEnter() {
    _followGPS = true;
    draw();
}

void MapsApp::latLngToTile(double lat, double lng, int zoom, int& tx, int& ty) {
    double n   = pow(2.0, zoom);
    tx = (int)((lng + 180.0) / 360.0 * n);
    double rad = lat * PI / 180.0;
    ty = (int)((1.0 - log(tan(rad) + 1.0/cos(rad)) / PI) / 2.0 * n);
}

void MapsApp::loadTileRaw(int tx, int ty, int zoom, uint8_t* buf) {
    char path[64];
    snprintf(path, sizeof(path), "%s/%d/%d/%d.raw", MAPS_PATH, zoom, tx, ty);
    File f = Storage::instance().open(path);
    if (!f) {
        // Fill with checkerboard pattern to show missing tile
        for (int i = 0; i < TILE_PX * TILE_PX; i++)
            buf[i] = ((i / TILE_PX + i % TILE_PX) & 1) ? C_LIGHT : C_WHITE;
        return;
    }
    // Raw file: TILE_PX×TILE_PX bytes, one byte per pixel, value 0-15
    size_t read = 0;
    while (read < (size_t)TILE_PX*TILE_PX && f.available())
        buf[read++] = f.read() & 0x0F;
    f.close();
}

void MapsApp::drawTile(int tx, int ty, int px, int py, int zoom) {
    Canvas& cv = Canvas::instance();
    // Allocate tile buffer in PSRAM
    uint8_t* tile = (uint8_t*)heap_caps_malloc(TILE_PX * TILE_PX, MALLOC_CAP_SPIRAM);
    if (!tile) return;

    loadTileRaw(tx, ty, zoom, tile);

    // Copy tile pixels to framebuffer, clipped to map area
    uint8_t* fb = cv.buf();
    for (int row = 0; row < TILE_PX; row++) {
        int fy = py + row;
        if (fy < MAP_Y || fy >= MAP_Y + MAP_H) continue;
        for (int col = 0; col < TILE_PX; col++) {
            int fx = px + col;
            if (fx < 0 || fx >= EPD_W) continue;
            fb[(size_t)fy * EPD_W + fx] = tile[row * TILE_PX + col];
        }
    }
    free(tile);
}

void MapsApp::drawMap() {
    Canvas& cv = Canvas::instance();
    cv.fillRect(0, MAP_Y, EPD_W, MAP_H, C_WHITE);
    drawNavBar("Maps");

    int baseTX, baseTY;
    latLngToTile(_centerLat, _centerLng, _zoom, baseTX, baseTY);

    // Pixel offset of tile origin relative to screen center
    int centerX = EPD_W / 2;
    int centerY = MAP_Y + MAP_H / 2;

    // Draw 3×3 tile grid around center tile
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int px = centerX + dx * TILE_PX - TILE_PX/2 + _panX;
            int py = centerY + dy * TILE_PX - TILE_PX/2 + _panY;
            drawTile(baseTX + dx, baseTY + dy, px, py, _zoom);
        }
    }

    // GPS position dot
    GPSFix fix = GPSHW::instance().fix();
    if (fix.valid) {
        int fx_tile, fy_tile;
        latLngToTile(fix.lat, fix.lng, _zoom, fx_tile, fy_tile);
        // Sub-tile fraction
        double n   = pow(2.0, _zoom);
        double fx_frac = ((fix.lng + 180.0) / 360.0 * n - fx_tile) * TILE_PX;
        double fy_frac = ((1.0 - log(tan(fix.lat*PI/180.0)+1.0/cos(fix.lat*PI/180.0))/PI)/2.0*n - fy_tile) * TILE_PX;
        int dotX = centerX + (fx_tile - baseTX) * TILE_PX + (int)fx_frac + _panX;
        int dotY = centerY + (fy_tile - baseTY) * TILE_PX + (int)fy_frac + _panY;
        cv.fillCircle(dotX, dotY, 8, C_BLACK);
        cv.drawCircle(dotX, dotY, 10, C_WHITE);
        cv.drawCircle(dotX, dotY, 11, C_BLACK);
    }

    drawCompass();
    drawGPSInfo();
}

void MapsApp::drawCompass() {
    Canvas& cv = Canvas::instance();
    int cx = EPD_W - 36, cy = MAP_Y + 36;
    cv.fillCircle(cx, cy, 24, C_WHITE);
    cv.drawCircle(cx, cy, 24, C_MID);
    // N arrow
    cv.drawLine(cx, cy - 18, cx,     cy - 4, C_BLACK);
    cv.drawLine(cx, cy - 18, cx - 5, cy - 8, C_BLACK);
    cv.drawLine(cx, cy - 18, cx + 5, cy - 8, C_BLACK);
    cv.drawText(cx - 3, cy - 24, "N", C_BLACK, C_WHITE, 1);
    // Zoom buttons
    cv.fillRoundRect(EPD_W - 50, MAP_Y + 80, 40, 30, 4, C_LIGHT);
    cv.drawTextCentered(EPD_W - 30, MAP_Y + 88, 40, "+", C_BLACK, C_LIGHT, 2);
    cv.fillRoundRect(EPD_W - 50, MAP_Y + 116, 40, 30, 4, C_LIGHT);
    cv.drawTextCentered(EPD_W - 30, MAP_Y + 124, 40, "-", C_BLACK, C_LIGHT, 2);
}

void MapsApp::drawGPSInfo() {
    Canvas& cv = Canvas::instance();
    GPSFix fix = GPSHW::instance().fix();
    int iy = EPD_H - DOCK_H - 48;
    cv.fillRoundRect(10, iy, 240, 42, 6, C_LIGHT);

    if (fix.valid) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%.5f, %.5f", fix.lat, fix.lng);
        cv.drawText(16, iy + 4,  buf, C_BLACK, C_LIGHT, 1);
        snprintf(buf, sizeof(buf), "%.0fm  %.1fkm/h  %dsats", fix.alt, fix.speed, fix.sats);
        cv.drawText(16, iy + 20, buf, C_BLACK, C_LIGHT, 1);
    } else {
        cv.drawText(16, iy + 14, "Searching for GPS...", C_MID, C_LIGHT, 1);
    }
}

void MapsApp::draw() {
    drawMap();
    Canvas::instance().flushFull();
}

void MapsApp::tick() {
    if (_followGPS && GPSHW::instance().hasFix()) {
        GPSFix fix = GPSHW::instance().fix();
        _centerLat = fix.lat;
        _centerLng = fix.lng;
        draw();
    }
}

void MapsApp::setZoom(int z) {
    _zoom  = constrain(z, 5, 18);
    _panX  = 0;
    _panY  = 0;
    draw();
}

void MapsApp::moveMap(int dx, int dy) {
    _panX += dx;
    _panY += dy;
    // Re-center when panned more than half a tile
    if (abs(_panX) > TILE_PX/2 || abs(_panY) > TILE_PX/2) {
        double n = pow(2.0, _zoom);
        _centerLng += ((double)_panX / TILE_PX) * (360.0 / n);
        double rad = _centerLat * PI / 180.0;
        _centerLat -= ((double)_panY / TILE_PX) * (180.0 / n);
        _centerLat  = constrain(_centerLat, -85.0, 85.0);
        _panX = 0;
        _panY = 0;
    }
    _followGPS = false;
    draw();
}

void MapsApp::handleEvent(const TouchEvent& ev) {
    if (isBackTap(ev)) { AppManager::instance().goHome(); return; }

    // Zoom buttons
    if (ev.type == TouchEvent::TAP) {
        if (ev.x > EPD_W-50 && ev.y > MAP_Y+80 && ev.y < MAP_Y+110)  { setZoom(_zoom+1); return; }
        if (ev.x > EPD_W-50 && ev.y > MAP_Y+116 && ev.y < MAP_Y+146) { setZoom(_zoom-1); return; }
        // Re-center on GPS
        if (ev.x < 80 && ev.y > EPD_H - DOCK_H - 60) { _followGPS = true; draw(); return; }
    }
    if (ev.type == TouchEvent::MOVE) {
        moveMap(ev.dx, ev.dy);
    }
    if (ev.type == TouchEvent::SWIPE_UP)   { setZoom(_zoom + 1); }
    if (ev.type == TouchEvent::SWIPE_DOWN) { setZoom(_zoom - 1); }
}
