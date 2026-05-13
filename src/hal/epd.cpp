#include "epd.h"

// ESP32-S3: FSPI=0 (SPI2), HSPI=1 (SPI3). EPD is on SPI0 pins → use FSPI.
SPIClass EPD::_spi(0);

// IT8951 SPI preambles
#define PRE_CMD  0x6000
#define PRE_WR   0x0000
#define PRE_RD   0x1000

// IT8951 registers / commands
#define IT8951_TCON_SYS_RUN    0x0001
#define IT8951_TCON_STANDBY    0x0002
#define IT8951_TCON_SLEEP      0x0003
#define IT8951_TCON_LD_IMG     0x0020
#define IT8951_TCON_LD_IMG_AREA 0x0021
#define IT8951_TCON_LD_IMG_END 0x0022
#define IT8951_I80CPCR         0x0004
#define IT8951_LISAR           0x0208  // Load image start address register

#define REG_BASE   0x1000
#define MCSR       (REG_BASE + 0x0000)
#define LISAR_L    (REG_BASE + 0x0208)
#define LISAR_H    (REG_BASE + 0x020A)

#define SPI_FREQ   20000000

EPD& EPD::instance() {
    static EPD inst;
    return inst;
}

bool EPD::waitBusy(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (digitalRead(EPD_BUSY) == LOW) {
        if (millis() - start > timeoutMs) {
            Serial.println("[EPD] waitBusy TIMEOUT");
            return false;
        }
        delay(1);
    }
    return true;
}

void EPD::writeCmd(uint16_t cmd) {
    waitBusy();
    _spi.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(EPD_CS, LOW);
    _spi.transfer16(PRE_CMD);
    _spi.transfer16(cmd);
    digitalWrite(EPD_CS, HIGH);
    _spi.endTransaction();
}

void EPD::writeWord(uint16_t data) {
    waitBusy();
    _spi.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(EPD_CS, LOW);
    _spi.transfer16(PRE_WR);
    _spi.transfer16(data);
    digitalWrite(EPD_CS, HIGH);
    _spi.endTransaction();
}

uint16_t EPD::readWord() {
    waitBusy();
    _spi.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(EPD_CS, LOW);
    _spi.transfer16(PRE_RD);
    _spi.transfer16(0);  // dummy word required by IT8951
    uint16_t v = _spi.transfer16(0);
    digitalWrite(EPD_CS, HIGH);
    _spi.endTransaction();
    return v;
}

uint16_t EPD::sendCmdArg(uint16_t cmd, const uint16_t* args, int n) {
    writeCmd(cmd);
    for (int i = 0; i < n - 1; i++) writeWord(args[i]);
    if (n > 0) {
        writeWord(args[n - 1]);
        return readWord();
    }
    return 0;
}

bool EPD::begin() {
    pinMode(EPD_CS,   OUTPUT); digitalWrite(EPD_CS,   HIGH);
    pinMode(EPD_RST,  OUTPUT);
    pinMode(EPD_BUSY, INPUT);

    _spi.begin(EPD_SCK, EPD_MISO, EPD_MOSI, EPD_CS);

    // Hardware reset — longer pulse for reliable IT8951 init
    digitalWrite(EPD_RST, LOW);  delay(200);
    digitalWrite(EPD_RST, HIGH); delay(200);

    if (!waitBusy(5000)) {
        Serial.println("[EPD] Not responding after reset");
        return false;
    }

    // SYS_RUN must come before GetDeviceInfo
    writeCmd(IT8951_TCON_SYS_RUN);
    delay(10);

    // Get device info (cmd 0x0302)
    writeCmd(0x0302);
    uint16_t w     = readWord();  // panel width
    uint16_t h     = readWord();  // panel height
    uint16_t addrL = readWord();  // imgBufAddr low word
    uint16_t addrH = readWord();  // imgBufAddr high word
    // skip firmware version (8 words) + LUT version (8 words)
    for (int i = 0; i < 16; i++) readWord();

    _panelW     = w ? w : EPD_W;
    _panelH     = h ? h : EPD_H;
    _imgBufAddr = ((uint32_t)addrH << 16) | addrL;  // correct byte order

    Serial.printf("[EPD] panel=%dx%d imgBuf=0x%08X\n", _panelW, _panelH, _imgBufAddr);

    // VCOM: set to -2.0V (typical for LILYGO 4.7" panel = 2000)
    writeCmd(0x0039);
    writeWord(2000);

    // Set image buffer base address
    writeCmd(0x0021);
    writeWord((uint16_t)(_imgBufAddr & 0xFFFF));
    writeCmd(0x0022);
    writeWord((uint16_t)(_imgBufAddr >> 16));

    // Setup frontlight PWM
    ledcSetup(FRONTLIGHT_CH, FRONTLIGHT_FREQ, FRONTLIGHT_RES);
    ledcAttachPin(FRONTLIGHT_PIN, FRONTLIGHT_CH);
    ledcWrite(FRONTLIGHT_CH, 0);

    clear();
    return true;
}

void EPD::setFrontlight(uint8_t brightness) {
    ledcWrite(FRONTLIGHT_CH, brightness);
}

void EPD::loadImgArea(const uint8_t* data, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    // Set image buffer address registers
    writeCmd(0x0021);
    writeWord((uint16_t)(_imgBufAddr & 0xFFFF));
    writeCmd(0x0022);
    writeWord((uint16_t)(_imgBufAddr >> 16));

    // Load image area command: rotation=0, bpp=4bpp(2), bigend=0
    uint16_t args[] = { 0x0002, x, y, w, h };
    writeCmd(IT8951_TCON_LD_IMG_AREA);
    for (auto a : args) writeWord(a);

    // Stream pixel data — IT8951 expects 4bpp packed (2 pixels/byte)
    waitBusy();
    _spi.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(EPD_CS, LOW);
    _spi.transfer16(PRE_WR);

    for (int row = 0; row < h; row++) {
        const uint8_t* line = data + (size_t)(y + row) * _panelW + x;
        for (int col = 0; col < w; col += 2) {
            // Convert 8bpp (0-15) to 4bpp packed: high nibble = left pixel
            uint8_t hi = (line[col]     & 0x0F) << 4;
            uint8_t lo = (col + 1 < w) ? (line[col + 1] & 0x0F) : 0;
            _spi.transfer(hi | lo);
        }
    }

    digitalWrite(EPD_CS, HIGH);
    _spi.endTransaction();

    writeCmd(IT8951_TCON_LD_IMG_END);
}

void EPD::displayArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, EPDMode mode) {
    writeCmd(0x0034);  // DPY_AREA
    writeWord(x);
    writeWord(y);
    writeWord(w);
    writeWord(h);
    writeWord((uint16_t)mode);
}

void EPD::clear() {
    // Fill with white using INIT waveform
    size_t sz = (size_t)_panelW * _panelH;
    uint8_t* tmp = (uint8_t*)heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
    if (tmp) {
        memset(tmp, C_WHITE, sz);
        loadImgArea(tmp, 0, 0, _panelW, _panelH);
        displayArea(0, 0, _panelW, _panelH, EPD_INIT);
        free(tmp);
    }
}

void EPD::flushFull(const uint8_t* fb, EPDMode mode) {
    loadImgArea(fb, 0, 0, _panelW, _panelH);
    displayArea(0, 0, _panelW, _panelH, mode);
}

void EPD::flushRegion(const uint8_t* fb, int x, int y, int w, int h, EPDMode mode) {
    // Align to even x for 4bpp packing
    if (x & 1) { x--; w++; }
    if (w & 1) w++;
    if (x + w > _panelW) w = _panelW - x;
    if (y + h > _panelH) h = _panelH - y;
    loadImgArea(fb, x, y, w, h);
    displayArea(x, y, w, h, mode);
}
