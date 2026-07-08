// test_hardening.cpp — robustness, security, and performance/stress coverage.
#include "TestFramework.hpp"
#include "../src/core/LayoutParser.hpp"
#include "../src/core/LayoutValidator.hpp"
#include "../src/core/KeyboardLayout.hpp"
#include "../src/core/KeyboardLayoutManager.hpp"
#include "../src/core/Composer.hpp"
#include "../src/core/Unicode.hpp"
#include "../src/core/VirtualKeys.hpp"
#include "../src/core/Stopwatch.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace core;

// ---- Security: invalid Unicode / surrogates are rejected --------------------

TEST(Hardening_SurrogateCodepointRejected)
{
    // U+D800 is a lone surrogate — must be refused, not injected.
    auto r = LayoutParser::parseString(R"({ "keys": { "A": ["U+D800"] } })");
    CHECK_FALSE(r.ok());
    CHECK(!r.errors.empty());
}

TEST(Hardening_OutOfRangeCodepointRejected)
{
    auto r = LayoutParser::parseString(R"({ "keys": { "A": ["0x110000"] } })");
    CHECK_FALSE(r.ok());
}

TEST(Hardening_ValidatorFlagsNonScalar)
{
    // Force a non-scalar into the layout model and confirm the validator errors.
    KeyboardLayout layout;
    KeyDef def{};
    def.levels[0].kind = ActionKind::Emit;
    def.levels[0].output = std::u32string{ 0xD800 }; // surrogate
    layout.setKey(vk::KeyA, def);
    auto rep = LayoutValidator::validate(layout);
    CHECK_FALSE(rep.ok());
}

// ---- Robustness: malformed JSON / corrupt input never crashes ---------------

TEST(Hardening_InvalidJsonForms)
{
    const char* bad[] = {
        "", "{", "}", "[1,2", "{\"keys\":}", "{ \"keys\": { \"A\": } }",
        "not json at all", "{ \"keys\": { \"A\": \"\\uZZZZ\" } }",
    };
    for (const char* j : bad) {
        auto r = LayoutParser::parseString(j);
        CHECK_FALSE(r.ok());          // rejected, and — crucially — no crash
    }
}

TEST(Hardening_InvalidUtf8Survives)
{
    // Truncated / illegal UTF-8 decodes to U+FFFD without over-reading.
    std::string junk = "\xFF\xFE\x41\xC0\x80\xED\xA0\x80";
    std::u32string out = unicode::utf8ToUtf32(junk);
    for (char32_t cp : out) CHECK(unicode::isValidScalar(cp));
    CHECK(!out.empty());
}

TEST(Hardening_Utf16RoundTripAllScalars)
{
    // Spot-check UTF-16 generation across BMP and SMP boundaries.
    for (char32_t cp : { char32_t(0x41), char32_t(0xFFFF), char32_t(0x10000),
                         char32_t(0x10F70), char32_t(0x10FFFF) }) {
        std::u16string u16 = unicode::utf32ToUtf16(std::u32string{ cp });
        std::u32string back = unicode::utf16ToUtf32(u16);
        CHECK_EQ(back.size(), size_t(1));
        CHECK_EQ(back[0], cp);
    }
}

// ---- Performance / stress ---------------------------------------------------

TEST(Hardening_LargeComposeTable)
{
    // 2000 compose sequences: build + lookup must stay correct and fast.
    KeyboardLayout layout;
    for (int i = 0; i < 2000; ++i) {
        std::u32string keys = { char32_t(0x10F70),
                                char32_t(0x100000 + i) }; // unique 2-cp sequences
        layout.addCompose(ComposeSequence{ keys, std::u32string{ char32_t(0x20 + (i % 90)) } });
    }
    std::u32string out;
    // Prefix of any sequence.
    CHECK(layout.composeLookup(std::u32string{ 0x10F70 }, out) == ComposeMatch::Prefix);
    // A specific exact sequence resolves.
    std::u32string exact = { char32_t(0x10F70), char32_t(0x100000 + 1234) };
    CHECK(layout.composeLookup(exact, out) == ComposeMatch::Exact);
    // A buffer longer than the longest sequence early-outs to None.
    std::u32string tooLong = { 0x10F70, 0x10F70, 0x10F70, 0x10F70 };
    CHECK(layout.composeLookup(tooLong, out) == ComposeMatch::None);

    // 100k lookups should complete near-instantly (index is O(1)); assert it
    // finishes within a generous bound rather than a brittle absolute time.
    Stopwatch sw;
    for (int i = 0; i < 100000; ++i)
        layout.composeLookup(std::u32string{ 0x10F70 }, out);
    CHECK(sw.millis() < 2000.0);
}

TEST(Hardening_LongTypingSession)
{
    // Feed thousands of keystrokes; the composition window must stay bounded and
    // the composer must never throw or corrupt state.
    auto r = LayoutParser::parseString(R"({
        "behavior": { "normalize": true },
        "keys": { "A": "U+10F70", "B": "U+10F71" }
    })");
    CHECK(r.ok());
    Composer c; c.setLayout(&*r.layout);
    for (int i = 0; i < 20000; ++i) {
        KeyInput k; k.vk = (i & 1) ? vk::KeyA : vk::KeyB;
        EmitOp op = c.process(k);
        CHECK(op.suppress);
    }
    CHECK(c.window().size() <= 32);   // bounded window, no unbounded growth
}

TEST(Hardening_RapidLayoutSwitching)
{
    // Build two layouts on disk and switch between them many times.
    fs::path dir = fs::temp_directory_path() / "auk_stress_switch";
    std::error_code ec; fs::remove_all(dir, ec); fs::create_directories(dir, ec);
    std::ofstream(dir / "one.json", std::ios::binary)
        << R"({ "meta": { "id": "one" }, "keys": { "A": "U+10F70" } })";
    std::ofstream(dir / "two.json", std::ios::binary)
        << R"({ "meta": { "id": "two" }, "keys": { "B": "U+10F71" } })";

    KeyboardLayoutManager mgr;
    CHECK(mgr.initialize(dir.string(), "one", nullptr));
    for (int i = 0; i < 500; ++i) {
        CHECK(mgr.switchTo((i & 1) ? "two" : "one"));
        CHECK(mgr.current() != nullptr);
    }
    CHECK_EQ(mgr.currentId(), std::string("two")); // last i=499 is odd -> "two"
    fs::remove_all(dir, ec);
}

TEST(Hardening_CorruptConfigTolerated)
{
    // The manager falls back to a valid layout when the preferred one is broken.
    fs::path dir = fs::temp_directory_path() / "auk_stress_corrupt";
    std::error_code ec; fs::remove_all(dir, ec); fs::create_directories(dir, ec);
    std::ofstream(dir / "good.json", std::ios::binary)
        << R"({ "meta": { "id": "good" }, "keys": { "A": "U+10F70" } })";
    std::ofstream(dir / "broken.json", std::ios::binary) << "{ this is not json";

    KeyboardLayoutManager mgr;
    // Prefer the broken one; initialize must still bring up a working layout.
    CHECK(mgr.initialize(dir.string(), "broken", nullptr));
    CHECK(mgr.current() != nullptr);
    CHECK_EQ(mgr.currentId(), std::string("good"));
    fs::remove_all(dir, ec);
}
