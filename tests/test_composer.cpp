#include "TestFramework.hpp"
#include "../src/core/Composer.hpp"
#include "../src/core/LayoutParser.hpp"
#include "../src/core/VirtualKeys.hpp"

using namespace core;

namespace {
// Shared layout used by most composer tests.
const char* kJson = R"({
    "behavior": { "caps_mode": "invert", "normalize": true },
    "keys": {
        "A": { "base": "U+10F70", "shift": "U+10F71" },
        "L": { "base": "U+10F78" },
        "OEM_4": { "base": "U+0308" },
        "OEM_3": { "base": { "dead": "d1" } },
        "OEM_5": { "base": { "compose": true } }
    },
    "dead_keys": {
        "d1": {
            "standalone": "U+10F84",
            "compose": { "U+10F70": ["U+10F70", "U+10F84"] }
        }
    },
    "ligatures": [
        { "sequence": ["U+10F78", "U+10F70"], "result": ["U+10FAA"] }
    ],
    "compose_sequences": [
        { "keys": ["U+10F70", "U+10F70"], "output": ["U+10F99"] }
    ]
})";

KeyInput key(unsigned vk, bool shift = false, bool caps = false)
{
    KeyInput k; k.vk = vk; k.shift = shift; k.caps = caps; return k;
}
} // namespace

TEST(Composer_BasicEmit)
{
    auto r = LayoutParser::parseString(kJson);
    CHECK(r.ok());
    Composer c; c.setLayout(&*r.layout);

    EmitOp op = c.process(key(vk::KeyA));
    CHECK(op.suppress);
    CHECK_EQ(op.backspaces, 0);
    CHECK(op.insert == std::u32string{ 0x10F70 });
}

TEST(Composer_ShiftLevel)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    EmitOp op = c.process(key(vk::KeyA, /*shift*/ true));
    CHECK(op.insert == std::u32string{ 0x10F71 });
}

TEST(Composer_CapsInvertsLetters)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    // caps on, shift off -> inverted to shift level.
    EmitOp op = c.process(key(vk::KeyA, /*shift*/ false, /*caps*/ true));
    CHECK(op.insert == std::u32string{ 0x10F71 });
}

TEST(Composer_DeadKeyCompose)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    EmitOp d = c.process(key(vk::OEM_3));   // dead key
    CHECK(d.suppress);
    CHECK(d.insert.empty());
    CHECK(c.deadPending());

    EmitOp a = c.process(key(vk::KeyA));     // compose with Aleph
    CHECK_FALSE(c.deadPending());
    CHECK(a.insert == (std::u32string{ 0x10F70, 0x10F84 }));
}

TEST(Composer_DeadKeyThenSpace_Standalone)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    c.process(key(vk::OEM_3));
    EmitOp sp = c.process(key(vk::Space));
    CHECK(sp.suppress);                      // space swallowed
    CHECK(sp.insert == std::u32string{ 0x10F84 });
}

TEST(Composer_DeadKeyThenUnmapped_FlushesStandalone)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    c.process(key(vk::OEM_3));
    EmitOp op = c.process(key(0x70));        // F1, unmapped
    CHECK_FALSE(op.suppress);                // physical key passes through
    CHECK(op.insert == std::u32string{ 0x10F84 });
    CHECK_FALSE(c.deadPending());
}

TEST(Composer_LigatureReconciles)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    EmitOp l = c.process(key(vk::KeyL));     // Lamedh
    CHECK(l.insert == std::u32string{ 0x10F78 });
    CHECK_EQ(l.backspaces, 0);

    EmitOp a = c.process(key(vk::KeyA));     // forms ligature L+A
    CHECK_EQ(a.backspaces, 1);               // delete the Lamedh
    CHECK(a.insert == std::u32string{ 0x10FAA });
}

TEST(Composer_NfcReconciles)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    // Type Aleph then a combining diaeresis mapped to OEM_4; NFC leaves them
    // as an ordered sequence (no precomposed form), so no backspaces here.
    c.process(key(vk::KeyA));                // Aleph
    EmitOp m = c.process(key(vk::OEM_4));    // U+0308 diaeresis
    CHECK(m.insert == std::u32string{ 0x0308 });
    CHECK_EQ(m.backspaces, 0);
}

TEST(Composer_BackspacePopsWindow)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    c.process(key(vk::KeyA));
    CHECK_EQ(c.window().size(), size_t(1));
    EmitOp bs = c.process(key(vk::Back));
    CHECK_FALSE(bs.suppress);                // app performs the delete
    CHECK_EQ(c.window().size(), size_t(0));
}

TEST(Composer_BackspaceCancelsDeadKey)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    c.process(key(vk::OEM_3));
    CHECK(c.deadPending());
    EmitOp bs = c.process(key(vk::Back));
    CHECK(bs.suppress);                      // swallow; only cancels the dead key
    CHECK_FALSE(c.deadPending());
}

TEST(Composer_SpaceIsBoundary)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    c.process(key(vk::KeyL));                // Lamedh
    EmitOp sp = c.process(key(vk::Space));
    CHECK_FALSE(sp.suppress);                // space passes through
    CHECK_EQ(c.window().size(), size_t(0));  // window reset at boundary

    // L <space> A must NOT ligature across the boundary.
    EmitOp a = c.process(key(vk::KeyA));
    CHECK_EQ(a.backspaces, 0);
    CHECK(a.insert == std::u32string{ 0x10F70 });
}

TEST(Composer_ComposeSequenceEmits)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    EmitOp start = c.process(key(vk::OEM_5));   // begin compose
    CHECK(start.suppress);
    CHECK(start.insert.empty());
    CHECK(c.composing());

    EmitOp a1 = c.process(key(vk::KeyA));        // first key: prefix, swallowed
    CHECK(a1.suppress);
    CHECK(a1.insert.empty());
    CHECK(c.composing());

    EmitOp a2 = c.process(key(vk::KeyA));        // completes A A -> U+10F99
    CHECK_FALSE(c.composing());
    CHECK(a2.insert == std::u32string{ 0x10F99 });
}

TEST(Composer_ComposeDeadEndAborts)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    c.process(key(vk::OEM_5));                    // begin compose
    c.process(key(vk::KeyA));                     // 'A' -> prefix of AA
    EmitOp l = c.process(key(vk::KeyL));          // 'AL' matches nothing -> abort
    CHECK_FALSE(c.composing());
    CHECK(l.insert.empty());                      // failed compose produces nothing
}

TEST(Composer_ComposeEscapeCancels)
{
    auto r = LayoutParser::parseString(kJson);
    Composer c; c.setLayout(&*r.layout);

    c.process(key(vk::OEM_5));
    c.process(key(vk::KeyA));
    EmitOp esc = c.process(key(vk::Escape));
    CHECK(esc.suppress);
    CHECK_FALSE(c.composing());
}
