#include "LayoutLoader.hpp"
#include "VirtualKeys.hpp"
#include "Unicode.hpp"

#include <fstream>
#include <sstream>
#include <map>
#include <cctype>

namespace core {

// ---------------------------------------------------------------------------
// Codepoint / output parsing helpers
// ---------------------------------------------------------------------------

std::optional<char32_t> parseCodepointToken(const json::Value& v, std::string& err)
{
    if (v.isNumber()) {
        int n = v.asInt(-1);
        if (n < 0 || n > 0x10FFFF) { err = "codepoint out of range"; return std::nullopt; }
        return static_cast<char32_t>(n);
    }
    if (v.isString()) {
        const std::string& s = v.asString();
        std::string t;
        for (char c : s) if (!std::isspace(static_cast<unsigned char>(c))) t += c;
        if (t.empty()) { err = "empty codepoint token"; return std::nullopt; }
        try {
            unsigned long cp;
            if (t.size() > 2 && (t[0] == 'U' || t[0] == 'u') && t[1] == '+')
                cp = std::stoul(t.substr(2), nullptr, 16);
            else if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
                cp = std::stoul(t.substr(2), nullptr, 16);
            else
                cp = std::stoul(t, nullptr, 10);
            if (cp > 0x10FFFF) { err = "codepoint out of range: " + s; return std::nullopt; }
            return static_cast<char32_t>(cp);
        } catch (...) {
            err = "invalid codepoint token: " + s;
            return std::nullopt;
        }
    }
    err = "codepoint must be a number or string";
    return std::nullopt;
}

std::optional<std::u32string> parseOutput(const json::Value& node, std::string& err)
{
    if (node.isString())
        return unicode::utf8ToUtf32(node.asString());

    if (node.isArray()) {
        std::u32string out;
        for (const auto& e : node.asArray()) {
            auto cp = parseCodepointToken(e, err);
            if (!cp) return std::nullopt;
            out += *cp;
        }
        return out;
    }
    if (node.isNumber()) {
        auto cp = parseCodepointToken(node, err);
        if (!cp) return std::nullopt;
        return std::u32string(1, *cp);
    }
    err = "output must be a string, number, or array of codepoints";
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// Builder
// ---------------------------------------------------------------------------

static const char* kLevelNames[] = { "base", "shift", "altgr", "shift_altgr" };

bool LayoutLoader::Builder::parseAction(const json::Value& node,
                                        const std::string& ctx, KeyAction& out)
{
    // A level may be:
    //   "text"                    -> Emit
    //   ["U+..","U+.."]           -> Emit (codepoints)
    //   { "emit": ... }           -> Emit
    //   { "dead": "acute" }       -> DeadKey
    if (node.isString() || node.isArray() || node.isNumber()) {
        std::string err;
        auto cps = parseOutput(node, err);
        if (!cps) { errors.push_back(ctx + ": " + err); return false; }
        out.kind = ActionKind::Emit;
        out.output = *cps;
        return true;
    }
    if (node.isObject()) {
        if (node.contains("dead")) {
            const auto& d = node["dead"];
            if (!d.isString()) { errors.push_back(ctx + ": 'dead' must be a string id"); return false; }
            out.kind = ActionKind::DeadKey;
            out.deadKey = d.asString();
            return true;
        }
        if (node.contains("emit")) {
            std::string err;
            auto cps = parseOutput(node["emit"], err);
            if (!cps) { errors.push_back(ctx + ": " + err); return false; }
            out.kind = ActionKind::Emit;
            out.output = *cps;
            if (node.contains("cased")) out.cased = node["cased"].asBool();
            return true;
        }
        errors.push_back(ctx + ": object action needs 'emit' or 'dead'");
        return false;
    }
    errors.push_back(ctx + ": unsupported action type");
    return false;
}

void LayoutLoader::Builder::parseMeta(const json::Value& v)
{
    auto& m = layout.meta();
    if (v["name"].isString())        m.name        = v["name"].asString();
    if (v["language"].isString())    m.language    = v["language"].asString();
    if (v["description"].isString()) m.description = v["description"].asString();
    if (v["author"].isString())      m.author      = v["author"].asString();
    if (v["version"].isNumber())     m.version     = v["version"].asInt(1);
}

void LayoutLoader::Builder::parseBehavior(const json::Value& v)
{
    // caps_mode: "ignore" | "shift_letters" | "invert"
    if (v["caps_mode"].isString()) {
        const std::string& c = v["caps_mode"].asString();
        if      (c == "ignore")        layout.setCapsMode(CapsMode::Ignore);
        else if (c == "shift_letters") layout.setCapsMode(CapsMode::ShiftLetters);
        else if (c == "invert")        layout.setCapsMode(CapsMode::Invert);
        else warnings.push_back("unknown caps_mode '" + c + "', using 'ignore'");
    }
    if (v["normalize"].isBool())
        layout.setNormalize(v["normalize"].asBool());
}

void LayoutLoader::Builder::parseDeadKeys(const json::Value& v)
{
    if (v.isNull()) return;
    if (!v.isObject()) { errors.push_back("'dead_keys' must be an object"); return; }

    for (const auto& [id, spec] : v.asObject()) {
        DeadKey dk;
        dk.id = id;
        std::string err;

        if (spec["standalone"].type() != json::Type::Null) {
            auto s = parseOutput(spec["standalone"], err);
            if (!s) { errors.push_back("dead_keys." + id + ".standalone: " + err); continue; }
            dk.standalone = *s;
        }
        const auto& comps = spec["compose"];
        if (!comps.isObject()) {
            warnings.push_back("dead_keys." + id + " has no 'compose' table");
        } else {
            for (const auto& [next, res] : comps.asObject()) {
                json::Value key(next); // reuse token parser for the map key
                auto cp = parseCodepointToken(key, err);
                if (!cp) {
                    // Allow single-character literal keys too.
                    auto u32 = unicode::utf8ToUtf32(next);
                    if (u32.size() == 1) cp = u32[0];
                    else { errors.push_back("dead_keys." + id + " key '" + next + "': " + err); continue; }
                }
                auto out = parseOutput(res, err);
                if (!out) { errors.push_back("dead_keys." + id + "['" + next + "']: " + err); continue; }
                dk.compositions[*cp] = *out;
            }
        }
        layout.addDeadKey(dk);
    }
}

void LayoutLoader::Builder::parseLigatures(const json::Value& v)
{
    if (v.isNull()) return;
    if (!v.isArray()) { errors.push_back("'ligatures' must be an array"); return; }

    for (const auto& e : v.asArray()) {
        std::string err;
        auto seq = parseOutput(e["sequence"], err);
        if (!seq) { errors.push_back("ligature.sequence: " + err); continue; }
        auto res = parseOutput(e["result"], err);
        if (!res) { errors.push_back("ligature.result: " + err); continue; }
        if (seq->empty()) { warnings.push_back("ligature with empty sequence skipped"); continue; }
        layout.addLigature(Ligature{ *seq, *res });
    }
}

void LayoutLoader::Builder::parseKeys(const json::Value& v)
{
    if (!v.isObject()) { errors.push_back("'keys' must be an object"); return; }

    // Duplicate detection:
    //  - hard dupe: same key name appears twice -> JSON map already merges,
    //    so instead we detect same VK reached via two different names.
    //  - soft dupe: same output glyph string produced by two (vk,level) slots.
    std::map<unsigned, std::string> vkToName;      // vk -> first name that set it
    std::map<std::u32string, std::string> emitted; // output -> "name/level"

    for (const auto& [name, node] : v.asObject()) {
        auto vk = vk::fromName(name);
        if (!vk) { warnings.push_back("keys: unrecognized key name '" + name + "', skipped"); continue; }

        if (auto it = vkToName.find(*vk); it != vkToName.end()) {
            errors.push_back("duplicate key mapping: '" + name + "' and '" +
                             it->second + "' both resolve to VK 0x" +
                             [&]{ std::ostringstream o; o << std::hex << *vk; return o.str(); }());
            continue;
        }
        vkToName[*vk] = name;

        KeyDef def{};
        bool anyCased = false;

        auto handleLevel = [&](Level lvl, const json::Value& lvlNode) {
            if (lvlNode.type() == json::Type::Null) return;
            KeyAction act{};
            std::string ctx = "keys." + name + "." + kLevelNames[static_cast<size_t>(lvl)];
            if (!parseAction(lvlNode, ctx, act)) return;

            // Mark letter keys as cased so Caps Lock can act on them.
            if (vk::isLetter(*vk)) { act.cased = true; anyCased = true; }

            // Soft duplicate check for Emit actions.
            if (act.kind == ActionKind::Emit && !act.output.empty()) {
                if (auto e = emitted.find(act.output); e != emitted.end()) {
                    warnings.push_back("duplicate output: " + ctx +
                                       " emits same glyph(s) as " + e->second);
                } else {
                    emitted[act.output] = ctx;
                }
            }
            def.levels[static_cast<size_t>(lvl)] = std::move(act);
        };

        if (node.isObject() && (node.contains("base") || node.contains("shift") ||
                                node.contains("altgr") || node.contains("shift_altgr") ||
                                node.contains("dead") || node.contains("emit"))) {
            // Object form: explicit levels, OR a single action object.
            if (node.contains("base") || node.contains("shift") ||
                node.contains("altgr") || node.contains("shift_altgr")) {
                handleLevel(Level::Base,       node["base"]);
                handleLevel(Level::Shift,      node["shift"]);
                handleLevel(Level::AltGr,      node["altgr"]);
                handleLevel(Level::ShiftAltGr, node["shift_altgr"]);
            } else {
                // Whole node is one action -> Base level.
                handleLevel(Level::Base, node);
            }
        } else {
            // Shorthand: the node itself is the Base-level output.
            handleLevel(Level::Base, node);
        }

        def.cased = anyCased;
        layout.setKey(*vk, def);
    }
}

void LayoutLoader::Builder::build(const json::Value& root)
{
    if (!root.isObject()) { errors.push_back("top-level JSON must be an object"); return; }

    if (root.contains("meta"))      parseMeta(root["meta"]);
    if (root.contains("behavior"))  parseBehavior(root["behavior"]);
    if (root.contains("dead_keys")) parseDeadKeys(root["dead_keys"]);
    if (root.contains("ligatures")) parseLigatures(root["ligatures"]);

    if (!root.contains("keys"))
        errors.push_back("layout has no 'keys' section");
    else
        parseKeys(root["keys"]);

    if (layout.keys().empty() && errors.empty())
        errors.push_back("layout defines no usable keys");
}

// ---------------------------------------------------------------------------
// Public entry points
// ---------------------------------------------------------------------------

LoadResult LayoutLoader::loadFromString(const std::string& jsonText)
{
    LoadResult result;
    json::Value root;
    try {
        root = json::parse(jsonText);
    } catch (const json::ParseError& e) {
        result.errors.push_back(std::string("JSON syntax: ") + e.what());
        return result;
    } catch (const std::exception& e) {
        result.errors.push_back(std::string("JSON error: ") + e.what());
        return result;
    }

    Builder b;
    b.build(root);
    result.errors   = std::move(b.errors);
    result.warnings = std::move(b.warnings);
    if (result.errors.empty())
        result.layout = std::move(b.layout);
    return result;
}

LoadResult LayoutLoader::loadFromFile(const std::string& path)
{
    LoadResult result;
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        result.errors.push_back("cannot open layout file: " + path);
        return result;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return loadFromString(ss.str());
}

} // namespace core
