#pragma once
#include <Arduino.h>
#include "zip_reader.h"

// Maximum number of ZIP entries we index (typical EPUB has < 200 files)
#define EPUB_MAX_ENTRIES  256
// Maximum spine items (chapters)
#define EPUB_MAX_CHAPTERS 128

struct EpubChapter {
    char id[64];         // manifest id
    char href[256];      // file path inside ZIP (relative to OPF dir)
    char title[128];     // extracted from <title> or id
};

struct EpubMeta {
    char title[128];
    char author[128];
    char language[16];
    char identifier[64];
};

class EpubParser {
public:
    EpubParser() = default;
    ~EpubParser() { close(); }

    // Open an EPUB file. Returns false if file cannot be parsed.
    bool      open(const char* path);
    void      close();
    bool      isOpen() const { return _zip.isOpen(); }

    const EpubMeta&    meta()         const { return _meta; }
    int                chapterCount() const { return _chapterCount; }
    const EpubChapter& chapter(int i) const { return _chapters[i]; }

    // Load chapter `i` as plain text into `out`.
    // Text is stripped of HTML tags, entities decoded, whitespace normalised.
    // Returns false if the chapter cannot be decompressed.
    bool      loadChapter(int idx, String& out);

private:
    bool      parseContainer(const String& xml);
    bool      parseOPF(const String& xml);
    void      buildChapterList(const String& opf,
                               const String& spineXml,
                               const String& manifestXml);

    ZipReader   _zip;
    ZipEntry    _entries[EPUB_MAX_ENTRIES];
    int         _entryCount = 0;

    EpubMeta    _meta{};
    EpubChapter _chapters[EPUB_MAX_CHAPTERS];
    int         _chapterCount = 0;

    // Directory prefix of the OPF file inside the EPUB ZIP
    // e.g. "OEBPS/" or "" if OPF is in root
    char        _opfDir[128] = "";
};
