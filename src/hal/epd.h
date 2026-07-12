#pragma once
#include <stdint.h>
#include "../config.h"

// Waveform hints kept for source compatibility with the UI layer.
// FastEPD chooses the actual passes; these map onto full vs. partial updates.
enum EPDMode : uint8_t {
    EPD_INIT = 0,   // full clear
    EPD_DU   = 1,   // fast / regional
    EPD_GC16 = 2,   // 16-gray quality (full)
    EPD_GL16 = 3,
    EPD_A2   = 6,
};

// Thin wrapper around the FastEPD parallel-eink driver (ED047TC1 + TPS65185).
// The rest of the OS renders into Canvas' 8bpp framebuffer and calls flush*().
class EPD {
public:
    static EPD& instance();

    bool    begin();
    void    clear();
    void    flushFull(const uint8_t* fb, EPDMode mode = EPD_GC16);
    void    flushRegion(const uint8_t* fb, int x, int y, int w, int h, EPDMode mode = EPD_DU);
    void    setFrontlight(uint8_t brightness);   // stored only — this board has no frontlight

    uint16_t panelW() const { return _panelW; }
    uint16_t panelH() const { return _panelH; }
    uint8_t  frontlight() const { return _frontlight; }
    bool     ok() const { return _ok; }

private:
    EPD() = default;
    uint16_t _panelW = EPD_W;
    uint16_t _panelH = EPD_H;
    uint8_t  _frontlight = 0;
    bool     _ok = false;
};
