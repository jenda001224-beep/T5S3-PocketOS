#pragma once
#include <SPI.h>
#include "../config.h"

// IT8951 waveform modes
enum EPDMode : uint8_t {
    EPD_INIT = 0,   // full clear
    EPD_DU   = 1,   // fast mono
    EPD_GC16 = 2,   // 16-gray quality
    EPD_GL16 = 3,   // 16-gray low flash
    EPD_A2   = 6,   // fastest mono, no flash
};

class EPD {
public:
    static EPD& instance();

    bool    begin();
    void    clear();
    // Flush a region of the framebuffer to the display
    void    flushFull(const uint8_t* fb, EPDMode mode = EPD_GC16);
    void    flushRegion(const uint8_t* fb, int x, int y, int w, int h, EPDMode mode = EPD_DU);
    void    setFrontlight(uint8_t brightness);   // 0-255

    uint32_t imgBufAddr() const { return _imgBufAddr; }
    uint16_t panelW() const { return _panelW; }
    uint16_t panelH() const { return _panelH; }

private:
    EPD() = default;
    void     writeCmd(uint16_t cmd);
    void     writeWord(uint16_t data);
    uint16_t readWord();
    void     waitBusy();
    uint16_t sendCmdArg(uint16_t cmd, const uint16_t* args, int n);
    void     loadImgArea(const uint8_t* data, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void     displayArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, EPDMode mode);

    uint32_t _imgBufAddr = 0;
    uint16_t _panelW = EPD_W;
    uint16_t _panelH = EPD_H;
    static SPIClass _spi;
};
