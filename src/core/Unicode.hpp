// Unicode.hpp — portable, header-only UTF-8 / UTF-16 / UTF-32 conversions.
//
// Win32-free so the mapping core and its unit tests build on any platform.
// All functions are total: invalid input is replaced with U+FFFD rather than
// throwing, which matches how a keyboard must behave (never crash on a key).
#pragma once

#include <string>
#include <cstdint>

namespace core::unicode {

constexpr char32_t kReplacement = 0xFFFD;

inline bool isSurrogate(char32_t cp)     { return cp >= 0xD800 && cp <= 0xDFFF; }
inline bool isHighSurrogate(char32_t cp) { return cp >= 0xD800 && cp <= 0xDBFF; }
inline bool isLowSurrogate(char32_t cp)  { return cp >= 0xDC00 && cp <= 0xDFFF; }
inline bool isValidScalar(char32_t cp)   { return cp <= 0x10FFFF && !isSurrogate(cp); }

// --- UTF-32 codepoint -> UTF-16 code units (1 or 2). Returns unit count. -----
inline int toUtf16(char32_t cp, char16_t out[2])
{
    if (!isValidScalar(cp)) cp = kReplacement;
    if (cp <= 0xFFFF) {
        out[0] = static_cast<char16_t>(cp);
        return 1;
    }
    cp -= 0x10000;
    out[0] = static_cast<char16_t>(0xD800 + (cp >> 10));
    out[1] = static_cast<char16_t>(0xDC00 + (cp & 0x3FF));
    return 2;
}

// --- Append one codepoint to a UTF-8 string. --------------------------------
inline void appendUtf8(std::string& s, char32_t cp)
{
    if (!isValidScalar(cp)) cp = kReplacement;
    if (cp <= 0x7F) {
        s += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        s += static_cast<char>(0xC0 | (cp >> 6));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        s += static_cast<char>(0xE0 | (cp >> 12));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        s += static_cast<char>(0xF0 | (cp >> 18));
        s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// --- Append one codepoint to a UTF-16 string. -------------------------------
inline void appendUtf16(std::u16string& s, char32_t cp)
{
    char16_t units[2];
    int n = toUtf16(cp, units);
    for (int i = 0; i < n; ++i) s += units[i];
}

// --- Decode a UTF-8 string into codepoints. Invalid bytes -> U+FFFD. ---------
inline std::u32string utf8ToUtf32(const std::string& s)
{
    std::u32string out;
    size_t i = 0, n = s.size();
    while (i < n) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        char32_t cp;
        int extra;
        if (c < 0x80)        { cp = c;         extra = 0; }
        else if ((c >> 5) == 0x6) { cp = c & 0x1F; extra = 1; }
        else if ((c >> 4) == 0xE) { cp = c & 0x0F; extra = 2; }
        else if ((c >> 3) == 0x1E){ cp = c & 0x07; extra = 3; }
        else { out += kReplacement; ++i; continue; }

        if (i + static_cast<size_t>(extra) >= n) { out += kReplacement; break; }
        bool ok = true;
        for (int k = 1; k <= extra; ++k) {
            unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if ((cc >> 6) != 0x2) { ok = false; break; }
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (!ok) {
            // Malformed continuation byte: emit one U+FFFD and resync at i+1.
            out += kReplacement; ++i; continue;
        }
        if (!isValidScalar(cp)) {
            // Structurally complete but illegal (surrogate/overlong/out-of-range):
            // consume the whole attempted sequence as a single U+FFFD.
            out += kReplacement; i += static_cast<size_t>(extra) + 1; continue;
        }
        out += cp;
        i += static_cast<size_t>(extra) + 1;
    }
    return out;
}

// --- Encode codepoints to UTF-8. --------------------------------------------
inline std::string utf32ToUtf8(const std::u32string& s)
{
    std::string out;
    for (char32_t cp : s) appendUtf8(out, cp);
    return out;
}

// --- UTF-16 <-> UTF-32. ------------------------------------------------------
inline std::u32string utf16ToUtf32(const std::u16string& s)
{
    std::u32string out;
    for (size_t i = 0; i < s.size(); ++i) {
        char32_t u = s[i];
        if (isHighSurrogate(u) && i + 1 < s.size() && isLowSurrogate(s[i + 1])) {
            char32_t lo = s[++i];
            out += 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
        } else if (isSurrogate(u)) {
            out += kReplacement;
        } else {
            out += u;
        }
    }
    return out;
}

inline std::u16string utf32ToUtf16(const std::u32string& s)
{
    std::u16string out;
    for (char32_t cp : s) appendUtf16(out, cp);
    return out;
}

// Number of UTF-16 code units a codepoint occupies (1 or 2). Useful for
// computing how many Backspaces a target application needs to delete it.
inline int utf16Length(char32_t cp) { return (isValidScalar(cp) && cp > 0xFFFF) ? 2 : 1; }

} // namespace core::unicode
