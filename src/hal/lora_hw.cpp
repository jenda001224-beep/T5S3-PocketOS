#include "lora_hw.h"

// LoRa shares the board SPI bus with the SD card (SD is brought up first in
// main.cpp, which calls SPI.begin on the board pins).
SX1262   LoRaHW::_radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, SPI);

static volatile bool _rxFlag = false;

void IRAM_ATTR LoRaHW::onReceive() { _rxFlag = true; }

LoRaHW& LoRaHW::instance() {
    static LoRaHW inst;
    return inst;
}

bool LoRaHW::begin() {
    // SPI bus already initialised by Storage::begin(); make sure it exists even
    // if the SD card was absent.
    SPI.begin(BOARD_SPI_SCLK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

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

void LoRaHW::setEnabled(bool on) {
    if (!_ready) return;
    _enabled = on;
    if (on) _radio.startReceive();
    else    _radio.sleep();
}

bool LoRaHW::setFrequency(float mhz) {
    if (!_ready) return false;
    int st = _radio.setFrequency(mhz);
    _radio.startReceive();
    return st == RADIOLIB_ERR_NONE;
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
