// UnicodeNames.hpp — codepoint <-> character-name lookup and search.
//
// A curated name table for the codepoints this keyboard works with: the Old
// Uyghur block (U+10F70..U+10F89) plus common ASCII/Latin and the combining
// marks used in composition. Like the normalizer, this is honestly scoped — it
// does not embed the whole Unicode Character Database — and is easy to extend.
// Portable and unit-tested.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace core {

struct UnicodeChar {
    char32_t    cp;
    std::string name;   // upper-case Unicode-style name
};

class UnicodeNames {
public:
    // Official-style name for a codepoint, or "" if not in the curated table.
    static std::string name(char32_t cp);

    // Exact codepoint lookup: returns the entry if known.
    static const UnicodeChar* byCodepoint(char32_t cp);

    // Case-insensitive substring search over names. Also matches a query that is
    // a codepoint spelling ("U+10F70", "0x10F70", "10F70"). Results are ordered
    // by codepoint; capped at `limit`.
    static std::vector<UnicodeChar> search(const std::string& query, size_t limit = 200);

    // The full curated table (ordered by codepoint).
    static const std::vector<UnicodeChar>& table();
};

} // namespace core
