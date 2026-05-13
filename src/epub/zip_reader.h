#pragma once
// Minimal ZIP reader that works directly on an SD File using zlib (included in ESP-IDF).
// Supports DEFLATE (method 8) and STORE (method 0) — the two methods used in EPUB.

#include <SD.h>
#include <Arduino.h>
// Use miniz from ESP-ROM as a drop-in zlib replacement (compatible API)
#include <rom/miniz.h>

struct ZipEntry {
    char     name[256];
    uint32_t offset;          // local file header offset
    uint32_t compSize;
    uint32_t uncompSize;
    uint16_t method;          // 0=STORE, 8=DEFLATE
    bool     valid;
};

class ZipReader {
public:
    ZipReader() = default;
    ~ZipReader() { close(); }

    bool     open(const char* path);
    void     close();
    bool     isOpen() const { return (bool)_file; }

    // Returns number of entries read into index (up to maxEntries).
    // Call once after open().
    int      buildIndex(ZipEntry* entries, int maxEntries);

    // Find an entry by name (exact or suffix match for flexibility).
    // Returns true and fills `out` if found.
    bool     findEntry(ZipEntry* entries, int count, const char* name, ZipEntry& out);

    // Decompress an entry into a heap-allocated buffer (caller must free).
    // Returns nullptr on failure. `outSize` is filled with the decompressed size.
    uint8_t* decompress(const ZipEntry& entry, size_t& outSize);

    // Decompress into a String (convenient for text files <= ~200 KB).
    bool     readText(const ZipEntry& entry, String& out);

private:
    // Locate End-of-Central-Directory record.
    bool     findEOCD(uint32_t& cdOffset, uint32_t& cdSize, uint16_t& numEntries);

    File     _file;
    uint32_t _fileSize = 0;
};
