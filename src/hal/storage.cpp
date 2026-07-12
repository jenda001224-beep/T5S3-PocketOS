#include "storage.h"

Storage& Storage::instance() {
    static Storage inst;
    return inst;
}

bool Storage::begin() {
    // SD shares the board SPI bus with the LoRa radio (MISO21 MOSI13 SCLK14).
    SPI.begin(BOARD_SPI_SCLK, BOARD_SPI_MISO, BOARD_SPI_MOSI, SD_CS);
    _mounted = SD.begin(SD_CS, SPI, 20000000);
    if (_mounted) {
        ensureDir(BOOKS_PATH);
        ensureDir(MAPS_PATH);
        ensureDir(NOTES_PATH);
        ensureDir(SKETCH_PATH);
        ensureDir("/pocket/saves");
    }
    return _mounted;
}

int Storage::listDir(const char* path, String* names, int maxNames, const char* ext) {
    if (!_mounted) return 0;
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) return 0;
    int count = 0;
    while (count < maxNames) {
        File f = dir.openNextFile();
        if (!f) break;
        String name = f.name();
        f.close();
        if (ext != nullptr) {
            String nameLower = name;
            nameLower.toLowerCase();
            String extLower  = ext;
            extLower.toLowerCase();
            if (!nameLower.endsWith(extLower)) continue;
        }
        names[count++] = name;
    }
    dir.close();
    return count;
}

bool Storage::exists(const char* path) {
    return _mounted && SD.exists(path);
}

size_t Storage::fileSize(const char* path) {
    if (!_mounted) return 0;
    File f = SD.open(path);
    size_t sz = f ? f.size() : 0;
    if (f) f.close();
    return sz;
}

File Storage::open(const char* path, const char* mode) {
    return SD.open(path, mode);
}

String Storage::readText(const char* path) {
    File f = SD.open(path);
    if (!f) return "";
    String s;
    s.reserve(f.size());
    while (f.available()) s += (char)f.read();
    f.close();
    return s;
}

bool Storage::writeText(const char* path, const String& data) {
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    f.print(data);
    f.close();
    return true;
}

bool Storage::ensureDir(const char* path) {
    if (!_mounted) return false;
    if (!SD.exists(path)) return SD.mkdir(path);
    return true;
}

uint64_t Storage::totalBytes() { return _mounted ? SD.totalBytes() : 0; }
uint64_t Storage::usedBytes()  { return _mounted ? SD.usedBytes()  : 0; }
