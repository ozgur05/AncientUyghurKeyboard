#include "LayoutSerializer.hpp"
#include "VirtualKeys.hpp"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

namespace core {

namespace {

// A codepoint as a "U+XXXX" token (min 4 hex digits, upper-case).
std::string cpToken(char32_t cp)
{
    std::ostringstream o;
    o << "U+" << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
      << static_cast<uint32_t>(cp);
    return o.str();
}

// A codepoint sequence as a JSON array of tokens: ["U+..","U+.."].
std::string cpArray(const std::u32string& s)
{
    std::string out = "[";
    for (size_t i = 0; i < s.size(); ++i) {
        if (i) out += ", ";
        out += '"';
        out += cpToken(s[i]);
        out += '"';
    }
    out += "]";
    return out;
}

// Escape a UTF-8 string for inclusion as a JSON string literal.
std::string jsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c & 0xFF);
                    out += buf;
                } else {
                    out += c; // pass UTF-8 bytes through unchanged
                }
        }
    }
    return out;
}

std::string quoted(const std::string& s) { return "\"" + jsonEscape(s) + "\""; }

const char* capsModeName(CapsMode m)
{
    switch (m) {
        case CapsMode::ShiftLetters: return "shift_letters";
        case CapsMode::Invert:       return "invert";
        case CapsMode::Ignore:
        default:                     return "ignore";
    }
}

const char* kLevelKeys[] = { "base", "shift", "altgr", "shift_altgr" };

// Serialize one KeyAction to its JSON value form.
std::string actionJson(const KeyAction& a)
{
    switch (a.kind) {
        case ActionKind::Emit:    return cpArray(a.output);
        case ActionKind::DeadKey: return "{ \"dead\": " + quoted(a.deadKey) + " }";
        case ActionKind::Compose: return "{ \"compose\": true }";
        case ActionKind::None:
        default:                  return "null";
    }
}

} // namespace

std::string LayoutSerializer::toJson(const KeyboardLayout& layout)
{
    std::ostringstream o;
    const std::string I1 = "  ", I2 = "    ", I3 = "      ";

    o << "{\n";

    // ---- meta ----
    const auto& m = layout.meta();
    o << I1 << "\"meta\": {\n";
    o << I2 << "\"id\": "          << quoted(m.id)          << ",\n";
    o << I2 << "\"name\": "        << quoted(m.name)        << ",\n";
    o << I2 << "\"language\": "    << quoted(m.language)    << ",\n";
    o << I2 << "\"description\": " << quoted(m.description) << ",\n";
    o << I2 << "\"author\": "      << quoted(m.author)      << ",\n";
    o << I2 << "\"version\": "     << m.version             << "\n";
    o << I1 << "},\n";

    // ---- behavior ----
    o << I1 << "\"behavior\": {\n";
    o << I2 << "\"caps_mode\": \"" << capsModeName(layout.capsMode()) << "\",\n";
    o << I2 << "\"normalize\": "   << (layout.normalize() ? "true" : "false") << "\n";
    o << I1 << "},\n";

    // ---- keys (sorted by VK for deterministic output) ----
    std::vector<unsigned> vks;
    vks.reserve(layout.keys().size());
    for (const auto& [vk, def] : layout.keys()) { (void)def; vks.push_back(vk); }
    std::sort(vks.begin(), vks.end());

    o << I1 << "\"keys\": {\n";
    for (size_t ki = 0; ki < vks.size(); ++ki) {
        const unsigned vk = vks[ki];
        const KeyDef& def = *layout.key(vk);
        o << I2 << quoted(vk::toName(vk)) << ": {";

        bool first = true;
        for (size_t lvl = 0; lvl < static_cast<size_t>(Level::Count); ++lvl) {
            const KeyAction& a = def.levels[lvl];
            if (a.kind == ActionKind::None) continue;
            o << (first ? " " : ", ") << "\"" << kLevelKeys[lvl] << "\": " << actionJson(a);
            first = false;
        }
        o << (first ? "}" : " }");
        o << (ki + 1 < vks.size() ? ",\n" : "\n");
    }
    o << I1 << "}";

    // ---- dead_keys ----
    if (!layout.deadKeys().empty()) {
        o << ",\n" << I1 << "\"dead_keys\": {\n";
        size_t di = 0, dn = layout.deadKeys().size();
        for (const auto& [id, dk] : layout.deadKeys()) {
            o << I2 << quoted(id) << ": {\n";
            o << I3 << "\"standalone\": " << cpArray(dk.standalone) << ",\n";
            o << I3 << "\"compose\": {";
            size_t ci = 0, cn = dk.compositions.size();
            for (const auto& [next, out] : dk.compositions) {
                o << (ci == 0 ? "\n" : "");
                o << I3 << "  \"" << cpToken(next) << "\": " << cpArray(out)
                  << (ci + 1 < cn ? ",\n" : "\n");
                ++ci;
            }
            o << (cn ? I3 : "") << "}\n";
            o << I2 << "}" << (di + 1 < dn ? ",\n" : "\n");
            ++di;
        }
        o << I1 << "}";
    }

    // ---- ligatures ----
    if (!layout.ligatures().empty()) {
        o << ",\n" << I1 << "\"ligatures\": [\n";
        const auto& ligs = layout.ligatures();
        for (size_t i = 0; i < ligs.size(); ++i) {
            o << I2 << "{ \"sequence\": " << cpArray(ligs[i].sequence)
              << ", \"result\": " << cpArray(ligs[i].result) << " }"
              << (i + 1 < ligs.size() ? ",\n" : "\n");
        }
        o << I1 << "]";
    }

    // ---- compose_sequences ----
    if (!layout.composeSequences().empty()) {
        o << ",\n" << I1 << "\"compose_sequences\": [\n";
        const auto& seqs = layout.composeSequences();
        for (size_t i = 0; i < seqs.size(); ++i) {
            o << I2 << "{ \"keys\": " << cpArray(seqs[i].keys)
              << ", \"output\": " << cpArray(seqs[i].output) << " }"
              << (i + 1 < seqs.size() ? ",\n" : "\n");
        }
        o << I1 << "]";
    }

    o << "\n}\n";
    return o.str();
}

} // namespace core
