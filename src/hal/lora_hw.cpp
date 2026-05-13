#include "lora_hw.h"

SPIClass LoRaHW::_spi(3);  // 3 = VSPI (renamed SPI3 in newer Arduino-ESP32)
SX1262   LoRaHW::_radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, LoRaHW::_spi);

static volatile bool _rxFlag = false;

void IRAM_ATTR LoRaHW::onReceive() { _rxFlag = true; }

LoRaHW& LoRaHW::instance() {
    static LoRaHW inst;
    return inst;
}

bool LoRaHW::begin() {
    _spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    int state = _radio.begin(
        LORA_FREQ,   // MHz
        125.0,       // bandwidth kHz
        10,          // spreading factor
        5,           // coding rate 4/5
        0x34,        // sync word
        17,          // dBm output power
        8            // preamble length
    );

    if (state != RADIOLIB_ERR_NONE) return false;

    _radio.setDio1Action(onReceive);
    _radio.startReceive();
    _ready = true;
    return true;
}

bool LoRaHW::send(const String& msg) {
    if (!_ready) return false;
    String tmp = msg;  // RadioLib transmit() needs non-const reference
    int state = _radio.transmit(tmp);
    _radio.startReceive();
    return state == RADIOLIB_ERR_NONE;
}

bool LoRaHW::available() {
    if (!_rxFlag) return false;
    _rxFlag = false;
    String s;
    int state = _radio.readData(s);
    if (state == RADIOLIB_ERR_NONE) {
        _pending.text = s;
        _pending.rssi = _radio.getRSSI();
        _pending.snr  = _radio.getSNR();
        _pending.ts   = millis();
        _pending.from = ""; // no routing yet
        _hasPacket = true;
        _lastRSSI  = _pending.rssi;
        _lastSNR   = _pending.snr;
    }
    _radio.startReceive();
    return _hasPacket;
}

LoRaPacket LoRaHW::receive() {
    _hasPacket = false;
    return _pending;
}
