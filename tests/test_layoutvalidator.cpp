#include "TestFramework.hpp"
#include "../src/core/LayoutParser.hpp"
#include "../src/core/LayoutValidator.hpp"

using namespace core;

static KeyboardLayout parse(const char* json)
{
    auto r = LayoutParser::parseString(json);
    return r.layout ? std::move(*r.layout) : KeyboardLayout{};
}

TEST(Validator_CleanLayout_Ok)
{
    auto layout = parse(R"({
        "keys": {
            "A": { "base": "U+10F70" },
            "OEM_3": { "base": { "dead": "d1" } }
        },
        "dead_keys": { "d1": { "standalone": "U+10F84",
                               "compose": { "U+10F70": "U+10F71" } } }
    })");
    auto rep = LayoutValidator::validate(layout);
    CHECK(rep.ok());
}

TEST(Validator_DanglingDeadKey_IsError)
{
    auto layout = parse(R"({
        "keys": { "OEM_3": { "base": { "dead": "missing" } }, "A": "U+10F70" }
    })");
    auto rep = LayoutValidator::validate(layout);
    CHECK_FALSE(rep.ok());
    CHECK(!rep.errors.empty());
}

TEST(Validator_DuplicateGlyph_IsWarning)
{
    auto layout = parse(R"({
        "keys": { "A": { "base": "U+10F70" }, "E": { "base": "U+10F70" } }
    })");
    auto rep = LayoutValidator::validate(layout);
    CHECK(rep.ok());              // not fatal
    CHECK(!rep.warnings.empty());
}

TEST(Validator_ComposePrefixAmbiguity_IsWarning)
{
    // "aa" is a prefix of "aab": the longer sequence can never fire.
    auto layout = parse(R"({
        "keys": { "A": "U+0061", "B": "U+0062" },
        "compose_sequences": [
            { "keys": ["U+0061","U+0061"],          "output": "U+00E5" },
            { "keys": ["U+0061","U+0061","U+0062"], "output": "U+00E6" }
        ]
    })");
    auto rep = LayoutValidator::validate(layout);
    CHECK(rep.ok());
    CHECK(!rep.warnings.empty());
}

TEST(Validator_EmptyLayout_IsError)
{
    KeyboardLayout empty;
    auto rep = LayoutValidator::validate(empty);
    CHECK_FALSE(rep.ok());
}
