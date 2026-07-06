// LayoutParser.hpp — build a KeyboardLayout from JSON (structural parsing).
//
// The parser is responsible for *syntax and structure*: turning a JSON document
// into a KeyboardLayout, reporting malformed values. Deeper semantic checks
// (dangling dead-key references, ambiguous compose sequences, duplicate glyphs)
// live in LayoutValidator, which runs on the produced layout.
#pragma once

#include "KeyboardLayout.hpp"
#include "../Json.hpp"

#include <string>
#include <vector>
#include <optional>

namespace core {

struct ParseResult {
    std::optional<KeyboardLayout> layout;    // present unless a fatal error occurred
    std::vector<std::string>      errors;
    std::vector<std::string>      warnings;

    bool ok() const { return layout.has_value() && errors.empty(); }
};

class LayoutParser {
public:
    static ParseResult parseString(const std::string& jsonText);
    static ParseResult parseFile(const std::string& path);

private:
    struct Builder {
        KeyboardLayout           layout;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        void build(const json::Value& root);
        void parseMeta(const json::Value& v);
        void parseBehavior(const json::Value& v);
        void parseDeadKeys(const json::Value& v);
        void parseLigatures(const json::Value& v);
        void parseCompose(const json::Value& v);
        void parseKeys(const json::Value& v);
        bool parseAction(const json::Value& node, const std::string& ctx, KeyAction& out);
    };
};

// --- Helpers (also used by tests) -------------------------------------------
std::optional<std::u32string> parseOutput(const json::Value& node, std::string& err);
std::optional<char32_t>       parseCodepointToken(const json::Value& v, std::string& err);

} // namespace core
