#include "epub_parser.h"
#include "xml_util.h"

bool EpubParser::open(const char* path) {
    if (!_zip.open(path)) return false;

    _entryCount   = _zip.buildIndex(_entries, EPUB_MAX_ENTRIES);
    _chapterCount = 0;
    memset(&_meta, 0, sizeof(_meta));

    // ── Step 1: Read META-INF/container.xml ───────────────────────────────────
    ZipEntry containerEntry;
    if (!_zip.findEntry(_entries, _entryCount, "META-INF/container.xml", containerEntry)) {
        // Try without path prefix
        if (!_zip.findEntry(_entries, _entryCount, "container.xml", containerEntry)) {
            close(); return false;
        }
    }
    String containerXml;
    if (!_zip.readText(containerEntry, containerXml)) { close(); return false; }

    if (!parseContainer(containerXml)) { close(); return false; }
    return true;
}

void EpubParser::close() {
    _zip.close();
    _entryCount   = 0;
    _chapterCount = 0;
}

bool EpubParser::parseContainer(const String& xml) {
    // <rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/>
    String opfPath = XML::attr(xml, "rootfile", "full-path");
    if (opfPath.isEmpty()) return false;

    // Derive OPF directory prefix
    int lastSlash = opfPath.lastIndexOf('/');
    if (lastSlash >= 0) {
        String dir = opfPath.substring(0, lastSlash + 1);
        strncpy(_opfDir, dir.c_str(), sizeof(_opfDir) - 1);
    } else {
        _opfDir[0] = '\0';
    }

    // Read OPF file
    ZipEntry opfEntry;
    if (!_zip.findEntry(_entries, _entryCount, opfPath.c_str(), opfEntry)) return false;

    String opfXml;
    if (!_zip.readText(opfEntry, opfXml)) return false;

    return parseOPF(opfXml);
}

bool EpubParser::parseOPF(const String& opf) {
    // ── Metadata ──────────────────────────────────────────────────────────────
    String title  = XML::text(opf, "dc:title");
    String author = XML::text(opf, "dc:creator");
    String lang   = XML::text(opf, "dc:language");
    String ident  = XML::text(opf, "dc:identifier");

    strncpy(_meta.title,      title.c_str(),  sizeof(_meta.title)  - 1);
    strncpy(_meta.author,     author.c_str(), sizeof(_meta.author) - 1);
    strncpy(_meta.language,   lang.c_str(),   sizeof(_meta.language) - 1);
    strncpy(_meta.identifier, ident.c_str(),  sizeof(_meta.identifier) - 1);

    // ── Manifest: id → href map ───────────────────────────────────────────────
    // We need to map idref (from spine) → href (actual file path).
    // Parse all <item> elements in <manifest>.
    struct ManifestItem { char id[64]; char href[256]; char mediaType[64]; };
    static ManifestItem manifest[EPUB_MAX_CHAPTERS];
    int mCount = 0;

    // Find <manifest> block
    int mStart = opf.indexOf("<manifest");
    int mEnd   = opf.indexOf("</manifest>", mStart);
    if (mStart < 0) return false;
    String manifestXml = (mEnd > 0)
        ? opf.substring(mStart, mEnd + 11)
        : opf.substring(mStart);

    // Collect all <item> entries
    String ids   [EPUB_MAX_CHAPTERS];
    String hrefs [EPUB_MAX_CHAPTERS];
    String mTypes[EPUB_MAX_CHAPTERS];

    int nIds    = XML::attrs(manifestXml, "item", "id",         ids,    EPUB_MAX_CHAPTERS);
    int nHrefs  = XML::attrs(manifestXml, "item", "href",       hrefs,  EPUB_MAX_CHAPTERS);
    int nMTypes = XML::attrs(manifestXml, "item", "media-type", mTypes, EPUB_MAX_CHAPTERS);

    mCount = min(min(nIds, nHrefs), EPUB_MAX_CHAPTERS);
    for (int i = 0; i < mCount; i++) {
        strncpy(manifest[i].id,        ids[i].c_str(),    63);
        strncpy(manifest[i].href,      hrefs[i].c_str(),  255);
        strncpy(manifest[i].mediaType, i < nMTypes ? mTypes[i].c_str() : "", 63);
    }

    // ── Spine: ordered list of idref ──────────────────────────────────────────
    int spStart = opf.indexOf("<spine");
    int spEnd   = opf.indexOf("</spine>", spStart);
    if (spStart < 0) return false;
    String spineXml = (spEnd > 0)
        ? opf.substring(spStart, spEnd + 8)
        : opf.substring(spStart);

    String idrefs[EPUB_MAX_CHAPTERS];
    int    nRefs = XML::attrs(spineXml, "itemref", "idref", idrefs, EPUB_MAX_CHAPTERS);

    // Build chapter list in spine order
    _chapterCount = 0;
    for (int s = 0; s < nRefs && _chapterCount < EPUB_MAX_CHAPTERS; s++) {
        // Find manifest entry matching this idref
        for (int m = 0; m < mCount; m++) {
            if (idrefs[s] == String(manifest[m].id)) {
                // Only include HTML/XHTML content
                String mt(manifest[m].mediaType);
                if (mt.indexOf("html") < 0 && mt.indexOf("xhtml") < 0 &&
                    mt != "" && mt.indexOf("xml") < 0) continue;

                EpubChapter& ch = _chapters[_chapterCount];
                strncpy(ch.id, manifest[m].id, 63);

                // href may be relative — prepend opfDir
                String fullHref = String(_opfDir) + String(manifest[m].href);
                // URL-decode %20 etc. (basic)
                fullHref.replace("%20", " ");
                // Remove fragment (#anchor)
                int hash = fullHref.indexOf('#');
                if (hash >= 0) fullHref = fullHref.substring(0, hash);

                strncpy(ch.href,  fullHref.c_str(), 255);
                strncpy(ch.title, manifest[m].id,   127);  // use id as placeholder title

                _chapterCount++;
                break;
            }
        }
    }

    // Try to set nicer chapter titles from NCX (optional)
    // Find ncx item in manifest
    for (int m = 0; m < mCount; m++) {
        String mt(manifest[m].mediaType);
        if (mt.indexOf("ncx") >= 0) {
            ZipEntry ncxEntry;
            String ncxPath = String(_opfDir) + String(manifest[m].href);
            if (_zip.findEntry(_entries, _entryCount, ncxPath.c_str(), ncxEntry)) {
                String ncx;
                if (_zip.readText(ncxEntry, ncx)) {
                    // Parse <navPoint> elements for labels
                    // This is a best-effort; we just update chapter titles
                    String labels[EPUB_MAX_CHAPTERS];
                    int n = XML::attrs(ncx, "text", "id", labels, EPUB_MAX_CHAPTERS);
                    // Actually extract navLabel > text content
                    int pos = 0;
                    int chIdx = 0;
                    while (chIdx < _chapterCount) {
                        int np = ncx.indexOf("<navPoint", pos);
                        if (np < 0) break;
                        int ne = ncx.indexOf("</navPoint>", np);
                        String navPt = (ne > 0) ? ncx.substring(np, ne) : ncx.substring(np);
                        String label = XML::text(navPt, "text");
                        if (label.length() > 0)
                            strncpy(_chapters[chIdx].title, label.c_str(), 127);
                        pos = (ne > 0) ? ne : (int)ncx.length();
                        chIdx++;
                    }
                }
            }
            break;
        }
    }

    return _chapterCount > 0;
}

bool EpubParser::loadChapter(int idx, String& out) {
    if (idx < 0 || idx >= _chapterCount) return false;

    ZipEntry entry;
    if (!_zip.findEntry(_entries, _entryCount, _chapters[idx].href, entry)) {
        // Try without opfDir prefix
        String bare = String(_chapters[idx].href).substring(strlen(_opfDir));
        if (!_zip.findEntry(_entries, _entryCount, bare.c_str(), entry)) return false;
    }

    String raw;
    if (!_zip.readText(entry, raw)) return false;

    out = XML::stripTags(raw);
    return true;
}
