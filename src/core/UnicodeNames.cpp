#include "UnicodeNames.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace core {

namespace {

std::string upper(const std::string& s)
{
    std::string o = s;
    for (char& c : o) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return o;
}

// Try to read a codepoint spelling: "U+10F70", "0x10F70", or bare hex "10F70".
bool asCodepoint(const std::string& q, char32_t& out)
{
    std::string t;
    for (char c : q) if (!std::isspace(static_cast<unsigned char>(c))) t += c;
    if (t.empty()) return false;
    std::string hex;
    if (t.size() > 2 && (t[0] == 'U' || t[0] == 'u') && t[1] == '+') hex = t.substr(2);
    else if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) hex = t.substr(2);
    else hex = t;
    if (hex.empty()) return false;
    for (char c : hex) if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    try {
        unsigned long v = std::stoul(hex, nullptr, 16);
        if (v > 0x10FFFF) return false;
        out = static_cast<char32_t>(v);
        return true;
    } catch (...) { return false; }
}

// Build the curated table once.
const std::vector<UnicodeChar>& buildTable()
{
    static const std::vector<UnicodeChar> t = [] {
        std::vector<UnicodeChar> v;

        // ASCII printable (U+0020..U+007E) with conventional names.
        auto add = [&](char32_t cp, const char* n) { v.push_back({ cp, n }); };
        add(0x0020, "SPACE");        add(0x0021, "EXCLAMATION MARK");
        add(0x0022, "QUOTATION MARK"); add(0x0023, "NUMBER SIGN");
        add(0x0024, "DOLLAR SIGN");   add(0x0025, "PERCENT SIGN");
        add(0x0026, "AMPERSAND");     add(0x0027, "APOSTROPHE");
        add(0x0028, "LEFT PARENTHESIS"); add(0x0029, "RIGHT PARENTHESIS");
        add(0x002A, "ASTERISK");      add(0x002B, "PLUS SIGN");
        add(0x002C, "COMMA");         add(0x002D, "HYPHEN-MINUS");
        add(0x002E, "FULL STOP");     add(0x002F, "SOLIDUS");
        for (char32_t c = 0x0030; c <= 0x0039; ++c)
            v.push_back({ c, "DIGIT " + std::string(1, static_cast<char>('0' + (c - 0x30))) });
        add(0x003A, "COLON");         add(0x003B, "SEMICOLON");
        add(0x003C, "LESS-THAN SIGN"); add(0x003D, "EQUALS SIGN");
        add(0x003E, "GREATER-THAN SIGN"); add(0x003F, "QUESTION MARK");
        add(0x0040, "COMMERCIAL AT");
        for (char32_t c = 0x0041; c <= 0x005A; ++c)
            v.push_back({ c, "LATIN CAPITAL LETTER " + std::string(1, static_cast<char>('A' + (c - 0x41))) });
        add(0x005B, "LEFT SQUARE BRACKET"); add(0x005C, "REVERSE SOLIDUS");
        add(0x005D, "RIGHT SQUARE BRACKET"); add(0x005E, "CIRCUMFLEX ACCENT");
        add(0x005F, "LOW LINE");      add(0x0060, "GRAVE ACCENT");
        for (char32_t c = 0x0061; c <= 0x007A; ++c)
            v.push_back({ c, "LATIN SMALL LETTER " + std::string(1, static_cast<char>('A' + (c - 0x61))) });
        add(0x007B, "LEFT CURLY BRACKET"); add(0x007C, "VERTICAL LINE");
        add(0x007D, "RIGHT CURLY BRACKET"); add(0x007E, "TILDE");

        // Common combining marks used in composition.
        add(0x0300, "COMBINING GRAVE ACCENT");
        add(0x0301, "COMBINING ACUTE ACCENT");
        add(0x0302, "COMBINING CIRCUMFLEX ACCENT");
        add(0x0303, "COMBINING TILDE");
        add(0x0308, "COMBINING DIAERESIS");
        add(0x0327, "COMBINING CEDILLA");

        // Old Uyghur block, U+10F70..U+10F89 (18 letters, 4 marks, 4 punct).
        add(0x10F70, "OLD UYGHUR LETTER ALEPH");
        add(0x10F71, "OLD UYGHUR LETTER BETH");
        add(0x10F72, "OLD UYGHUR LETTER GIMEL-HETH");
        add(0x10F73, "OLD UYGHUR LETTER WAW");
        add(0x10F74, "OLD UYGHUR LETTER ZAYIN");
        add(0x10F75, "OLD UYGHUR LETTER FINAL HETH");
        add(0x10F76, "OLD UYGHUR LETTER YODH");
        add(0x10F77, "OLD UYGHUR LETTER KAPH");
        add(0x10F78, "OLD UYGHUR LETTER LAMEDH");
        add(0x10F79, "OLD UYGHUR LETTER MEM");
        add(0x10F7A, "OLD UYGHUR LETTER NUN");
        add(0x10F7B, "OLD UYGHUR LETTER SAMEKH");
        add(0x10F7C, "OLD UYGHUR LETTER PE");
        add(0x10F7D, "OLD UYGHUR LETTER SADHE");
        add(0x10F7E, "OLD UYGHUR LETTER RESH");
        add(0x10F7F, "OLD UYGHUR LETTER SHIN");
        add(0x10F80, "OLD UYGHUR LETTER TAW");
        add(0x10F81, "OLD UYGHUR LETTER LESH");
        add(0x10F82, "OLD UYGHUR COMBINING DOT ABOVE");
        add(0x10F83, "OLD UYGHUR COMBINING DOT BELOW");
        add(0x10F84, "OLD UYGHUR COMBINING TWO DOTS ABOVE");
        add(0x10F85, "OLD UYGHUR COMBINING TWO DOTS BELOW");
        add(0x10F86, "OLD UYGHUR PUNCTUATION BAR");
        add(0x10F87, "OLD UYGHUR PUNCTUATION TWO BARS");
        add(0x10F88, "OLD UYGHUR PUNCTUATION TWO DOTS");
        add(0x10F89, "OLD UYGHUR PUNCTUATION FOUR DOTS");

        std::sort(v.begin(), v.end(),
                  [](const UnicodeChar& a, const UnicodeChar& b) { return a.cp < b.cp; });
        return v;
    }();
    return t;
}

const std::unordered_map<char32_t, size_t>& index()
{
    static const std::unordered_map<char32_t, size_t> idx = [] {
        std::unordered_map<char32_t, size_t> m;
        const auto& t = buildTable();
        for (size_t i = 0; i < t.size(); ++i) m[t[i].cp] = i;
        return m;
    }();
    return idx;
}

} // namespace

const std::vector<UnicodeChar>& UnicodeNames::table() { return buildTable(); }

const UnicodeChar* UnicodeNames::byCodepoint(char32_t cp)
{
    const auto& idx = index();
    auto it = idx.find(cp);
    return it == idx.end() ? nullptr : &buildTable()[it->second];
}

std::string UnicodeNames::name(char32_t cp)
{
    const UnicodeChar* c = byCodepoint(cp);
    return c ? c->name : std::string();
}

std::vector<UnicodeChar> UnicodeNames::search(const std::string& query, size_t limit)
{
    std::vector<UnicodeChar> out;
    const auto& t = buildTable();

    // Codepoint spelling: return the exact entry (plus a synthetic entry if the
    // codepoint is a valid scalar not in the table, so the picker can still use it).
    char32_t cp;
    if (asCodepoint(query, cp)) {
        if (const UnicodeChar* c = byCodepoint(cp)) out.push_back(*c);
        else if (cp <= 0x10FFFF && !(cp >= 0xD800 && cp <= 0xDFFF))
            out.push_back({ cp, "(unnamed codepoint)" });
        return out;
    }

    // Name substring (case-insensitive).
    const std::string q = upper(query);
    if (q.empty()) {
        for (const auto& c : t) { out.push_back(c); if (out.size() >= limit) break; }
        return out;
    }
    for (const auto& c : t) {
        if (c.name.find(q) != std::string::npos) {
            out.push_back(c);
            if (out.size() >= limit) break;
        }
    }
    return out;
}

} // namespace core
