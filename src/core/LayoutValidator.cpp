#include "LayoutValidator.hpp"
#include "Unicode.hpp"

#include <map>
#include <set>
#include <sstream>
#include <cstdint>

namespace core {

namespace {
std::string vkHex(unsigned vk)
{
    std::ostringstream o; o << "0x" << std::hex << vk; return o.str();
}
std::string glyphStr(const std::u32string& s)
{
    // Human-readable "U+XXXX U+YYYY" for diagnostics.
    std::ostringstream o;
    for (size_t i = 0; i < s.size(); ++i) {
        if (i) o << ' ';
        o << "U+" << std::hex << std::uppercase << static_cast<uint32_t>(s[i]);
    }
    return o.str();
}

// True if every codepoint in `s` is a valid Unicode scalar (in range, not a
// surrogate). Anything else would produce broken UTF-16 when injected.
bool allScalar(const std::u32string& s)
{
    for (char32_t cp : s)
        if (!unicode::isValidScalar(cp)) return false;
    return true;
}
} // namespace

ValidationReport LayoutValidator::validate(const KeyboardLayout& layout)
{
    ValidationReport rep;

    if (layout.keys().empty())
        rep.errors.push_back("layout has no keys");

    // 1. Dead-key references + duplicate glyph detection across all key levels.
    std::map<std::u32string, std::string> firstEmitter;
    for (const auto& [vk, def] : layout.keys()) {
        for (size_t lvl = 0; lvl < static_cast<size_t>(Level::Count); ++lvl) {
            const KeyAction& a = def.levels[lvl];
            if (a.kind == ActionKind::DeadKey) {
                if (!layout.deadKey(a.deadKey))
                    rep.errors.push_back("key VK " + vkHex(vk) +
                        " references undefined dead key '" + a.deadKey + "'");
            } else if (a.kind == ActionKind::Emit && !a.output.empty()) {
                std::string where = "VK " + vkHex(vk) + " level " + std::to_string(lvl);
                if (!allScalar(a.output))
                    rep.errors.push_back("key " + where +
                        " emits a non-scalar codepoint (surrogate/out-of-range)");
                auto it = firstEmitter.find(a.output);
                if (it != firstEmitter.end())
                    rep.warnings.push_back("glyph " + glyphStr(a.output) +
                        " emitted by both " + it->second + " and " + where);
                else
                    firstEmitter[a.output] = where;
            }
        }
    }

    // 2. Dead keys: at least one composition, and all outputs must be scalar.
    for (const auto& [id, dk] : layout.deadKeys()) {
        if (dk.compositions.empty())
            rep.warnings.push_back("dead key '" + id + "' has no compositions");
        if (!allScalar(dk.standalone))
            rep.errors.push_back("dead key '" + id + "' standalone is non-scalar");
        for (const auto& [next, out] : dk.compositions)
            if (!allScalar(out))
                rep.errors.push_back("dead key '" + id + "' has a non-scalar output");
    }

    // 3. Compose sequences: duplicates and prefix ambiguity.
    const auto& seqs = layout.composeSequences();
    std::set<std::u32string> seen;
    for (const auto& s : seqs) {
        if (s.keys.empty() || s.output.empty()) {
            rep.warnings.push_back("compose sequence with empty keys/output");
            continue;
        }
        if (!allScalar(s.keys) || !allScalar(s.output))
            rep.errors.push_back("compose sequence has a non-scalar codepoint");
        if (!seen.insert(s.keys).second)
            rep.warnings.push_back("duplicate compose sequence " + glyphStr(s.keys));
    }
    // Exact-that-is-also-a-prefix -> the longer sequence is unreachable.
    for (const auto& a : seqs) {
        for (const auto& b : seqs) {
            if (&a == &b) continue;
            if (b.keys.size() > a.keys.size() &&
                b.keys.compare(0, a.keys.size(), a.keys) == 0) {
                rep.warnings.push_back("compose sequence " + glyphStr(b.keys) +
                    " is unreachable: shorter " + glyphStr(a.keys) + " matches first");
            }
        }
    }

    return rep;
}

} // namespace core
