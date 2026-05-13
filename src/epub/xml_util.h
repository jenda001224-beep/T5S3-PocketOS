#pragma once
// Tiny XML helpers — no full parser, just targeted string searches.
// Good enough for well-formed OPF/container.xml files.
#include <Arduino.h>

namespace XML {

// Returns the value of `attr` in the first occurrence of `<tag ... attr="VALUE" ...>`.
// E.g. xmlAttr(xml, "rootfile", "full-path") → "OEBPS/content.opf"
String attr(const String& xml, const char* tag, const char* attr);

// Collects all values of `attr` from every <tag ...> element.
// E.g. xmlAttrs(xml, "itemref", "idref") → {"ch01","ch02",...}
int    attrs(const String& xml, const char* tag, const char* attr,
             String* out, int maxOut);

// Returns text content between <tag> and </tag>.
String text(const String& xml, const char* tag);

// Strip all XML/HTML tags; decode basic entities.
String stripTags(const String& html);

} // namespace XML
