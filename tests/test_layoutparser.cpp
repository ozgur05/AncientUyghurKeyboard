#include "TestFramework.hpp"
#include "../src/core/LayoutParser.hpp"
#include "../src/core/VirtualKeys.hpp"

using namespace core;

TEST(Parser_Minimal)
{
    const char* json = R"({
        "meta": { "name": "Test", "version": 2 },
        "keys": { "A": { "base": "U+10F70" } }
    })";
    auto r = LayoutParser::parseString(json);
    CHECK(r.ok());
    if (r.layout) {
        CHECK_EQ(r.layout->meta().name, std::string("Test"));
        CHECK_EQ(r.layout->meta().version, 2);
        const KeyDef* k = r.layout->key(vk::KeyA);
        CHECK(k != nullptr);
        if (k) {
            CHECK(k->levels[0].kind == ActionKind::Emit);
            CHECK(k->levels[0].output == std::u32string{ 0x10F70 });
        }
    }
}

TEST(Parser_ShorthandAndCodepointForms)
{
    const char* json = R"({
        "keys": {
            "A": "U+10F70",
            "B": { "base": ["0x10F71"] },
            "C": { "base": 4466 },
            "D": { "base": "x" }
        }
    })";
    auto r = LayoutParser::parseString(json);
    CHECK(r.ok());
    if (r.layout) {
        CHECK(r.layout->key(vk::KeyA)->levels[0].output == std::u32string{ 0x10F70 });
        CHECK(r.layout->key(vk::KeyB)->levels[0].output == std::u32string{ 0x10F71 });
        CHECK(r.layout->key(vk::KeyC)->levels[0].output == std::u32string{ 4466 });
        CHECK(r.layout->key(vk::KeyD)->levels[0].output == std::u32string{ U'x' });
    }
}

TEST(Parser_DuplicateKeyName_IsError)
{
    // "A" and "0x41" both resolve to VK 0x41 -> hard duplicate error.
    const char* json = R"({
        "keys": {
            "A":    { "base": "U+10F70" },
            "0x41": { "base": "U+10F71" }
        }
    })";
    auto r = LayoutParser::parseString(json);
    CHECK_FALSE(r.ok());
    CHECK(!r.errors.empty());
}

TEST(Parser_BadJson_IsError)
{
    auto r = LayoutParser::parseString("{ not valid json ");
    CHECK_FALSE(r.ok());
    CHECK(!r.errors.empty());
    CHECK_FALSE(r.layout.has_value());
}

TEST(Parser_NoKeys_IsError)
{
    auto r = LayoutParser::parseString(R"({ "meta": { "name": "x" } })");
    CHECK_FALSE(r.ok());
}

TEST(Parser_DeadKeysLigaturesCompose)
{
    const char* json = R"({
        "keys": {
            "A": { "base": "U+10F70" },
            "OEM_3": { "base": { "dead": "d1" } },
            "OEM_5": { "base": { "compose": true } }
        },
        "dead_keys": {
            "d1": { "standalone": "U+10F84", "compose": { "U+10F70": ["U+10F70","U+10F84"] } }
        },
        "ligatures": [ { "sequence": ["U+10F78","U+10F70"], "result": ["U+10F70"] } ],
        "compose_sequences": [ { "keys": ["U+10F70","U+10F70"], "output": ["U+10F71"] } ]
    })";
    auto r = LayoutParser::parseString(json);
    CHECK(r.ok());
    if (r.layout) {
        CHECK(r.layout->key(vk::OEM_3)->levels[0].kind == ActionKind::DeadKey);
        CHECK(r.layout->key(vk::OEM_5)->levels[0].kind == ActionKind::Compose);
        CHECK(r.layout->deadKey("d1") != nullptr);
        CHECK_EQ(r.layout->ligatures().size(), size_t(1));
        CHECK_EQ(r.layout->composeSequences().size(), size_t(1));
    }
}

TEST(Parser_Behavior_CapsAndNormalize)
{
    const char* json = R"({
        "behavior": { "caps_mode": "invert", "normalize": false },
        "keys": { "A": { "base": "U+10F70" } }
    })";
    auto r = LayoutParser::parseString(json);
    CHECK(r.ok());
    if (r.layout) {
        CHECK(r.layout->capsMode() == CapsMode::Invert);
        CHECK_FALSE(r.layout->normalize());
    }
}

TEST(Parser_MetadataFields)
{
    // Every metadata field the layout format promises must round-trip.
    const char* json = R"({
        "meta": {
            "id": "old_uyghur_x", "name": "Name", "language": "oui",
            "description": "Desc", "author": "Auth", "version": 7
        },
        "keys": { "A": "U+10F70" }
    })";
    auto r = LayoutParser::parseString(json);
    CHECK(r.ok());
    if (r.layout) {
        const auto& m = r.layout->meta();
        CHECK_EQ(m.id, std::string("old_uyghur_x"));
        CHECK_EQ(m.name, std::string("Name"));
        CHECK_EQ(m.language, std::string("oui"));
        CHECK_EQ(m.description, std::string("Desc"));
        CHECK_EQ(m.author, std::string("Auth"));
        CHECK_EQ(m.version, 7);
    }
}

TEST(Parser_UnknownKeyName_WarnsButLoads)
{
    // Invalid key validation: an unrecognized key name is skipped with a
    // warning; the rest of the layout still loads.
    auto r = LayoutParser::parseString(
        R"({ "keys": { "A": "U+10F70", "NotAKey": "U+10F71" } })");
    CHECK(r.ok());
    CHECK(!r.warnings.empty());
    if (r.layout) CHECK(r.layout->key(vk::KeyA) != nullptr);
}

TEST(Parser_AltGrAndShiftAltGrLevels)
{
    const char* json = R"({
        "keys": { "A": {
            "base": "U+10F70", "shift": "U+10F71",
            "altgr": "U+10F72", "shift_altgr": "U+10F73"
        } }
    })";
    auto r = LayoutParser::parseString(json);
    CHECK(r.ok());
    if (r.layout) {
        const KeyDef* k = r.layout->key(vk::KeyA);
        CHECK(k != nullptr);
        if (k) {
            CHECK(k->levels[(int)Level::AltGr].output      == std::u32string{ 0x10F72 });
            CHECK(k->levels[(int)Level::ShiftAltGr].output == std::u32string{ 0x10F73 });
        }
    }
}
