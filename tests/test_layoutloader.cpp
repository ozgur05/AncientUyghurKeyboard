#include "TestFramework.hpp"
#include "../src/core/LayoutLoader.hpp"
#include "../src/core/VirtualKeys.hpp"

using namespace core;

TEST(Loader_Minimal)
{
    const char* json = R"({
        "meta": { "name": "Test", "version": 2 },
        "keys": { "A": { "base": "U+10F70" } }
    })";
    auto r = LayoutLoader::loadFromString(json);
    CHECK(r.ok());
    CHECK(r.layout.has_value());
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

TEST(Loader_ShorthandAndCodepointForms)
{
    // Shorthand base, array codepoints, decimal, and literal string all work.
    const char* json = R"({
        "keys": {
            "A": "U+10F70",
            "B": { "base": ["0x10F71"] },
            "C": { "base": 4466 },
            "D": { "base": "x" }
        }
    })";
    auto r = LayoutLoader::loadFromString(json);
    CHECK(r.ok());
    if (r.layout) {
        CHECK(r.layout->key(vk::KeyA)->levels[0].output == std::u32string{ 0x10F70 });
        CHECK(r.layout->key(vk::KeyB)->levels[0].output == std::u32string{ 0x10F71 });
        CHECK(r.layout->key(vk::KeyC)->levels[0].output == std::u32string{ 4466 });
        CHECK(r.layout->key(vk::KeyD)->levels[0].output == std::u32string{ U'x' });
    }
}

TEST(Loader_DuplicateMapping_IsError)
{
    // "A" and "0x41" both resolve to VK 0x41 -> hard duplicate error.
    const char* json = R"({
        "keys": {
            "A":    { "base": "U+10F70" },
            "0x41": { "base": "U+10F71" }
        }
    })";
    auto r = LayoutLoader::loadFromString(json);
    CHECK_FALSE(r.ok());
    CHECK(!r.errors.empty());
}

TEST(Loader_DuplicateGlyph_IsWarning)
{
    // Two different keys emit the same glyph -> warning, still loads.
    const char* json = R"({
        "keys": {
            "A": { "base": "U+10F70" },
            "E": { "base": "U+10F70" }
        }
    })";
    auto r = LayoutLoader::loadFromString(json);
    CHECK(r.ok());
    CHECK(!r.warnings.empty());
}

TEST(Loader_BadJson_IsError)
{
    auto r = LayoutLoader::loadFromString("{ not valid json ");
    CHECK_FALSE(r.ok());
    CHECK(!r.errors.empty());
    CHECK_FALSE(r.layout.has_value());
}

TEST(Loader_NoKeys_IsError)
{
    auto r = LayoutLoader::loadFromString(R"({ "meta": { "name": "x" } })");
    CHECK_FALSE(r.ok());
}

TEST(Loader_DeadKeysAndLigatures)
{
    const char* json = R"({
        "keys": { "A": { "base": "U+10F70" }, "OEM_3": { "base": { "dead": "d1" } } },
        "dead_keys": {
            "d1": { "standalone": "U+10F84", "compose": { "U+10F70": ["U+10F70","U+10F84"] } }
        },
        "ligatures": [ { "sequence": ["U+10F78","U+10F70"], "result": ["U+10F70"] } ]
    })";
    auto r = LayoutLoader::loadFromString(json);
    CHECK(r.ok());
    if (r.layout) {
        const DeadKey* dk = r.layout->deadKey("d1");
        CHECK(dk != nullptr);
        if (dk) {
            CHECK(dk->standalone == std::u32string{ 0x10F84 });
            auto it = dk->compositions.find(0x10F70);
            CHECK(it != dk->compositions.end());
        }
        CHECK_EQ(r.layout->ligatures().size(), size_t(1));
    }
}

TEST(Loader_Behavior_CapsAndNormalize)
{
    const char* json = R"({
        "behavior": { "caps_mode": "invert", "normalize": false },
        "keys": { "A": { "base": "U+10F70" } }
    })";
    auto r = LayoutLoader::loadFromString(json);
    CHECK(r.ok());
    if (r.layout) {
        CHECK(r.layout->capsMode() == CapsMode::Invert);
        CHECK_FALSE(r.layout->normalize());
    }
}
