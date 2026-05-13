#include "zip_reader.h"
#include <esp_heap_caps.h>

// ─── Little-endian helpers ────────────────────────────────────────────────────
static uint16_t ru16(const uint8_t* p) { return p[0] | ((uint16_t)p[1] << 8); }
static uint32_t ru32(const uint8_t* p) { return p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }

bool ZipReader::open(const char* path) {
    _file = SD.open(path, "r");
    if (!_file) return false;
    _fileSize = _file.size();
    return true;
}

void ZipReader::close() {
    if (_file) _file.close();
}

bool ZipReader::findEOCD(uint32_t& cdOffset, uint32_t& cdSize, uint16_t& numEntries) {
    // EOCD is in the last 65557 bytes. Scan backwards for signature 0x06054b50.
    const uint32_t SIG = 0x06054b50;
    uint32_t searchStart = (_fileSize > 65557) ? _fileSize - 65557 : 0;
    uint8_t buf[22];

    for (int32_t pos = (int32_t)_fileSize - 22; pos >= (int32_t)searchStart; pos--) {
        _file.seek(pos);
        _file.read(buf, 22);
        if (ru32(buf) == SIG) {
            numEntries = ru16(buf + 10);
            cdSize     = ru32(buf + 12);
            cdOffset   = ru32(buf + 16);
            return true;
        }
    }
    return false;
}

int ZipReader::buildIndex(ZipEntry* entries, int maxEntries) {
    uint32_t cdOffset; uint32_t cdSize; uint16_t numEntries;
    if (!findEOCD(cdOffset, cdSize, numEntries)) return 0;

    _file.seek(cdOffset);
    int count = 0;

    for (int i = 0; i < numEntries && count < maxEntries; i++) {
        uint8_t hdr[46];
        if (_file.read(hdr, 46) != 46) break;
        if (ru32(hdr) != 0x02014b50) break;  // central dir sig

        uint16_t method    = ru16(hdr + 10);
        uint32_t compSz    = ru32(hdr + 20);
        uint32_t uncompSz  = ru32(hdr + 24);
        uint16_t nameLen   = ru16(hdr + 28);
        uint16_t extraLen  = ru16(hdr + 30);
        uint16_t commentLen= ru16(hdr + 32);
        uint32_t localOff  = ru32(hdr + 42);

        ZipEntry& e = entries[count];
        e.valid      = true;
        e.method     = method;
        e.compSize   = compSz;
        e.uncompSize = uncompSz;
        e.offset     = localOff;

        int nl = min((int)nameLen, 255);
        _file.read((uint8_t*)e.name, nl);
        e.name[nl] = '\0';

        // Skip extra + comment
        _file.seek(_file.position() + (nameLen - nl) + extraLen + commentLen);
        count++;
    }
    return count;
}

bool ZipReader::findEntry(ZipEntry* entries, int count, const char* name, ZipEntry& out) {
    String target(name);
    for (int i = 0; i < count; i++) {
        String n(entries[i].name);
        // Exact match OR suffix match (handles path prefixes in EPUB)
        if (n == target || n.endsWith(target) || n.endsWith("/" + target)) {
            out = entries[i];
            return true;
        }
    }
    return false;
}

uint8_t* ZipReader::decompress(const ZipEntry& entry, size_t& outSize) {
    // Skip local file header
    _file.seek(entry.offset);
    uint8_t lhdr[30];
    _file.read(lhdr, 30);
    if (ru32(lhdr) != 0x04034b50) return nullptr;
    uint16_t lNameLen  = ru16(lhdr + 26);
    uint16_t lExtraLen = ru16(lhdr + 28);
    _file.seek(entry.offset + 30 + lNameLen + lExtraLen);

    // Read compressed data
    uint8_t* comp = (uint8_t*)heap_caps_malloc(entry.compSize, MALLOC_CAP_SPIRAM);
    if (!comp) return nullptr;
    _file.read(comp, entry.compSize);

    if (entry.method == 0) {
        // STORE — no compression
        outSize = entry.uncompSize;
        return comp;
    }

    // DEFLATE — use tinfl_decompress_mem_to_mem (ESP-ROM low-level API).
    // MINIZ_NO_ZLIB_APIS is set in esp-rom miniz.h, so high-level inflate() is unavailable.
    // Raw DEFLATE: no zlib header → flag = TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF
    uint8_t* out = (uint8_t*)heap_caps_malloc(entry.uncompSize + 1, MALLOC_CAP_SPIRAM);
    if (!out) { free(comp); return nullptr; }

    size_t result = tinfl_decompress_mem_to_mem(
        out, entry.uncompSize,
        comp, entry.compSize,
        TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF  // raw deflate, output fits in one buffer
    );
    free(comp);

    if (result == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) { free(out); return nullptr; }

    out[entry.uncompSize] = '\0';
    outSize = entry.uncompSize;
    return out;
}

bool ZipReader::readText(const ZipEntry& entry, String& result) {
    size_t sz;
    uint8_t* data = decompress(entry, sz);
    if (!data) return false;
    // Build String in chunks to avoid stack overflow
    result = "";
    result.reserve(min(sz, (size_t)65536));
    for (size_t i = 0; i < sz; i++) result += (char)data[i];
    free(data);
    return true;
}
