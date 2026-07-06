// LayoutLoader.hpp — build a KeyboardLayout from JSON, with validation.
//
// The loader is deliberately strict-but-informative: structural errors abort
// the load (returns nullopt) with a message list; softer problems (e.g. a key
// name that doesn't resolve, or a duplicate glyph mapping) are recorded as
// warnings but still allow a usable layout to load.
#pragma once

#include "KeyboardLayout.hpp"
#include "../Json.hpp"

#include <string>
#include <vector>
#include <optional>

namespace core {

struct LoadResult {
    std::optional<KeyboardLayout> layout;      // present on success
    std::vector<std::string>      errors;      // fatal problems
    std::vector<std::string>      warnings;    // non-fatal problems

    bool ok() const { return layout.has_value() && errors.empty(); }
};

class LayoutLoader {
public:
    // Parse a JSON layout from an in-memory UTF-8 string.
    static LoadResult loadFromString(const std::string& jsonText);

    // Read a file (UTF-8) and parse it. File errors are reported in `errors`.
    static LoadResult loadFromFile(const std::string& path);

private:
    // Internal builder holding accumulating diagnostics.
    struct Builder {
        KeyboardLayout           layout;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        // Tracks which (vk, level) pairs are already assigned to catch dupes,
        // and which output glyph strings are already produced (soft dupes).
        void build(const json::Value& root);

        void parseMeta(const json::Value& v);
        void parseBehavior(const json::Value& v);
        void parseDeadKeys(const json::Value& v);
        void parseLigatures(const json::Value& v);
        void parseKeys(const json::Value& v);

        // Convert a JSON action node into a KeyAction. Returns false on error.
        bool parseAction(const json::Value& node, const std::string& ctx,
                         KeyAction& out);
    };
};

// --- Helpers exposed for testing --------------------------------------------

// Convert an "output" JSON node to codepoints. Accepts:
//   "abc"                        -> literal UTF-8 text
//   ["U+10F70", "0x10F71", 4465] -> explicit codepoints
std::optional<std::u32string> parseOutput(const json::Value& node, std::string& err);

// Parse a single codepoint token: "U+10F70", "0x10F70", or a JSON number.
std::optional<char32_t> parseCodepointToken(const json::Value& v, std::string& err);

} // namespace core
