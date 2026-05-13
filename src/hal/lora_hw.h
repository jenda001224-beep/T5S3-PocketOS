#pragma once
#include <RadioLib.h>
#include <SPI.h>
#include "../config.h"

struct LoRaPacket {
    String  from;
    String  text;
    int8_t  rssi;
    float   snr;
    uint32_t ts;
};

class LoRaHW {
public:
    static LoRaHW& instance();
    bool   begin();
    bool   send(const String& msg);
    bool   available();
    LoRaPacket receive();
    int8_t rssi()  const { return _lastRSSI; }
    float  snr()   const { return _lastSNR; }
    bool   ready() const { return _ready; }

private:
    LoRaHW() = default;
    static SPIClass _spi;
    static SX1262   _radio;
    bool   _ready    = false;
    bool   _hasPacket = false;
    int8_t _lastRSSI = 0;
    float  _lastSNR  = 0;
    LoRaPacket _pending;
    static void IRAM_ATTR onReceive();
};
