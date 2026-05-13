#include "xml_util.h"

namespace XML {

// Case-insensitive indexOf helper
static int findCI(const String& s, const char* needle, int from = 0) {
    String sl = s.substring(from); sl.toLowerCase();
    String nl(needle);             nl.toLowerCase();
    int idx = sl.indexOf(nl);
    return idx < 0 ? -1 : idx + from;
}

String attr(const String& xml, const char* tag, const char* attrName) {
    int pos = 0;
    while (true) {
        // Find opening of tag
        String openTag = String("<") + tag;
        int tagPos = findCI(xml, openTag.c_str(), pos);
        if (tagPos < 0) return "";

        // Find end of this tag
        int closePos = xml.indexOf('>', tagPos);
        if (closePos < 0) return "";

        String tagStr = xml.substring(tagPos, closePos + 1);

        // Look for attr="..." or attr='...' inside tag
        String aStr = String(attrName) + "=";
        int aPos = findCI(tagStr, aStr.c_str());
        if (aPos >= 0) {
            aPos += aStr.length();
            char quote = tagStr[aPos];
            if (quote == '"' || quote == '\'') {
                int end = tagStr.indexOf(quote, aPos + 1);
                if (end >= 0) return tagStr.substring(aPos + 1, end);
            }
        }
        pos = closePos + 1;
    }
}

int attrs(const String& xml, const char* tag, const char* attrName,
          String* out, int maxOut) {
    int count = 0;
    int pos   = 0;
    String openTag = String("<") + tag;

    while (count < maxOut) {
        int tagPos = findCI(xml, openTag.c_str(), pos);
        if (tagPos < 0) break;

        int closePos = xml.indexOf('>', tagPos);
        if (closePos < 0) break;

        String tagStr = xml.substring(tagPos, closePos + 1);
        String aStr   = String(attrName) + "=";
        int aPos = findCI(tagStr, aStr.c_str());
        if (aPos >= 0) {
            aPos += aStr.length();
            char quote = tagStr[aPos];
            if (quote == '"' || quote == '\'') {
                int end = tagStr.indexOf(quote, aPos + 1);
                if (end >= 0) out[count++] = tagStr.substring(aPos + 1, end);
            }
        }
        pos = closePos + 1;
    }
    return count;
}

String text(const String& xml, const char* tag) {
    String open  = String("<")  + tag;
    String close = String("</") + tag + ">";
    int s = findCI(xml, open.c_str());
    if (s < 0) return "";
    s = xml.indexOf('>', s);
    if (s < 0) return "";
    s++;
    int e = findCI(xml, close.c_str(), s);
    if (e < 0) return "";
    return xml.substring(s, e);
}

// Decode common HTML entities in-place
static String decodeEntities(const String& in) {
    String out;
    out.reserve(in.length());
    for (int i = 0; i < (int)in.length(); ) {
        if (in[i] != '&') { out += in[i++]; continue; }
        // Check common entities
        if      (in.substring(i,i+4) == "&lt;")   { out += '<';  i+=4; }
        else if (in.substring(i,i+4) == "&gt;")   { out += '>';  i+=4; }
        else if (in.substring(i,i+5) == "&amp;")  { out += '&';  i+=5; }
        else if (in.substring(i,i+6) == "&nbsp;") { out += ' ';  i+=6; }
        else if (in.substring(i,i+6) == "&apos;") { out += '\''; i+=6; }
        else if (in.substring(i,i+6) == "&quot;") { out += '"';  i+=6; }
        else if (in[i+1] == '#') {
            // Numeric entity &#NNN; or &#xHH;
            int semi = in.indexOf(';', i+2);
            if (semi > 0) {
                long code = 0;
                if (in[i+2] == 'x' || in[i+2] == 'X')
                    code = strtol(in.substring(i+3, semi).c_str(), nullptr, 16);
                else
                    code = strtol(in.substring(i+2, semi).c_str(), nullptr, 10);
                // Basic latin printable range
                if (code >= 32 && code < 127) out += (char)code;
                else if (code == 0x2019 || code == 0x0027) out += '\'';
                else if (code == 0x201C || code == 0x201D) out += '"';
                else if (code == 0x2014) out += '-';
                else if (code == 0x2013) out += '-';
                else if (code == 0x00A0) out += ' ';
                i = semi + 1;
            } else { out += in[i++]; }
        } else { out += in[i++]; }
    }
    return out;
}

String stripTags(const String& html) {
    String out;
    out.reserve(html.length());

    bool inTag    = false;
    bool inScript = false;
    bool inStyle  = false;
    char tagBuf[32]; int tbLen = 0;

    for (int i = 0; i < (int)html.length(); i++) {
        char c = html[i];

        if (!inTag) {
            if (c == '<') {
                inTag = true; tbLen = 0;
                // Peek at closing tag for script/style
                String peek = html.substring(i+1, i+8);
                peek.toLowerCase();
                if (peek.startsWith("/script")) inScript = false;
                if (peek.startsWith("/style"))  inStyle  = false;
                continue;
            }
            if (!inScript && !inStyle) out += c;
        } else {
            if (c == '>') {
                inTag = false;
                // Capture tag name
                String tname(tagBuf, min(tbLen, 31));
                tname.toLowerCase();
                tname.trim();
                if (tname.startsWith("script")) inScript = true;
                if (tname.startsWith("style"))  inStyle  = true;
                // Block-level elements → newline
                if (tname == "p"   || tname == "/p"  ||
                    tname == "div" || tname == "/div" ||
                    tname == "br"  || tname == "br/"  ||
                    tname == "h1"  || tname == "h2"   ||
                    tname == "h3"  || tname == "h4"   ||
                    tname == "/h1" || tname == "/h2"  ||
                    tname == "/h3" || tname == "/h4"  ||
                    tname == "li"  || tname == "/li"  ||
                    tname == "tr"  || tname == "/tr") {
                    if (!inScript && !inStyle) out += '\n';
                }
            } else {
                if (tbLen < 31) tagBuf[tbLen++] = c;
                tagBuf[tbLen < 31 ? tbLen : 31] = '\0';
            }
        }
    }

    // Collapse runs of whitespace / blank lines
    String result;
    result.reserve(out.length());
    int blanks  = 0;
    bool lastNL = false;
    for (int i = 0; i < (int)out.length(); i++) {
        char c = out[i];
        if (c == '\r') continue;
        if (c == '\n') {
            if (!lastNL || blanks < 1) { result += '\n'; blanks++; }
            lastNL = true;
        } else {
            lastNL = false; blanks = 0;
            result += c;
        }
    }

    return decodeEntities(result);
}

} // namespace XML
