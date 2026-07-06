#include "Normalizer.hpp"

#include <unordered_map>
#include <map>
#include <utility>
#include <algorithm>

namespace core::normalizer {

// --- Canonical combining classes (curated) ----------------------------------
int combiningClass(char32_t cp)
{
    static const std::unordered_map<char32_t, int> kCcc = {
        // Common Latin combining marks (Unicode General Category Mn).
        {0x0300, 230}, {0x0301, 230}, {0x0302, 230}, {0x0303, 230},
        {0x0304, 230}, {0x0306, 230}, {0x0307, 230}, {0x0308, 230},
        {0x030A, 230}, {0x030B, 230}, {0x030C, 230}, {0x0327, 202},
        {0x0328, 202}, {0x0323, 220}, {0x0324, 220}, {0x0325, 220},
        {0x0331, 220}, {0x0332, 220},
        // Old Uyghur combining marks (U+10F82..U+10F85).
        {0x10F82, 230}, // COMBINING DOT ABOVE
        {0x10F83, 220}, // COMBINING DOT BELOW
        {0x10F84, 230}, // COMBINING TWO DOTS ABOVE
        {0x10F85, 220}, // COMBINING TWO DOTS BELOW
    };
    auto it = kCcc.find(cp);
    return it == kCcc.end() ? 0 : it->second;
}

// --- Canonical composition table (curated) ----------------------------------
static char32_t composePair(char32_t a, char32_t b)
{
    static const std::map<std::pair<char32_t, char32_t>, char32_t> kCompose = {
        {{0x0041, 0x0308}, 0x00C4}, {{0x0061, 0x0308}, 0x00E4}, // A/a + diaeresis
        {{0x004F, 0x0308}, 0x00D6}, {{0x006F, 0x0308}, 0x00F6}, // O/o + diaeresis
        {{0x0055, 0x0308}, 0x00DC}, {{0x0075, 0x0308}, 0x00FC}, // U/u + diaeresis
        {{0x0041, 0x0301}, 0x00C1}, {{0x0061, 0x0301}, 0x00E1}, // A/a + acute
        {{0x0045, 0x0301}, 0x00C9}, {{0x0065, 0x0301}, 0x00E9}, // E/e + acute
        {{0x0041, 0x0300}, 0x00C0}, {{0x0061, 0x0300}, 0x00E0}, // A/a + grave
        {{0x004E, 0x0303}, 0x00D1}, {{0x006E, 0x0303}, 0x00F1}, // N/n + tilde
        {{0x0043, 0x0327}, 0x00C7}, {{0x0063, 0x0327}, 0x00E7}, // C/c + cedilla
    };
    auto it = kCompose.find({a, b});
    return it == kCompose.end() ? 0 : it->second;
}

std::u32string canonicalOrder(const std::u32string& s)
{
    std::u32string out = s;
    size_t i = 0, n = out.size();
    while (i < n) {
        if (combiningClass(out[i]) == 0) { ++i; continue; }
        // Find the extent of this combining-mark run.
        size_t j = i;
        while (j < n && combiningClass(out[j]) != 0) ++j;
        // Stable bubble sort by combining class (stability preserves same-class order).
        for (size_t a = i; a < j; ++a)
            for (size_t b = i; b + 1 < j - (a - i); ++b)
                if (combiningClass(out[b]) > combiningClass(out[b + 1]))
                    std::swap(out[b], out[b + 1]);
        i = j;
    }
    return out;
}

std::u32string compose(const std::u32string& s)
{
    std::u32string out;
    out.reserve(s.size());
    int  lastStarter = -1;   // index in `out` of the last starter
    int  lastCcc     = -1;   // ccc of the most recent appended mark (blocking)

    for (char32_t cp : s) {
        int ccc = combiningClass(cp);

        if (lastStarter >= 0 && ccc > 0 && ccc > lastCcc) {
            char32_t composed = composePair(out[static_cast<size_t>(lastStarter)], cp);
            if (composed != 0) {
                out[static_cast<size_t>(lastStarter)] = composed;
                continue; // mark absorbed; starter/ccc bookkeeping unchanged
            }
        }

        out += cp;
        if (ccc == 0) { lastStarter = static_cast<int>(out.size()) - 1; lastCcc = 0; }
        else          { lastCcc = ccc; }
    }
    return out;
}

std::u32string toNFC(const std::u32string& s)
{
    return compose(canonicalOrder(s));
}

} // namespace core::normalizer
