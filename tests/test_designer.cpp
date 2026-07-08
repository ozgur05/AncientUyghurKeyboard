// test_designer.cpp — designer core: serializer, document, unicode names, keycaps.
#include "TestFramework.hpp"
#include "../src/core/LayoutParser.hpp"
#include "../src/core/LayoutSerializer.hpp"
#include "../src/core/LayoutDocument.hpp"
#include "../src/core/UnicodeNames.hpp"
#include "../src/core/KeyCaps.hpp"
#include "../src/core/VirtualKeys.hpp"

using namespace core;

// ---- Serializer round-trip ---------------------------------------------------

static KeyAction emit(std::u32string s) { KeyAction a; a.kind = ActionKind::Emit; a.output = std::move(s); return a; }

TEST(Serializer_RoundTrip_Full)
{
    const char* json = R"({
        "meta": { "id": "rt", "name": "Round Trip", "language": "oui",
                  "description": "d", "author": "a", "version": 5 },
        "behavior": { "caps_mode": "invert", "normalize": false },
        "keys": {
            "A": { "base": "U+10F70", "shift": "U+10F71" },
            "S": { "base": ["U+10F7B"], "shift": { "dead": "d1" } },
            "OEM_2": { "altgr": { "compose": true } }
        },
        "dead_keys": { "d1": { "standalone": ["U+10F84"],
                               "compose": { "U+10F70": ["U+10F70","U+10F84"] } } },
        "ligatures": [ { "sequence": ["U+10F78","U+10F70"], "result": ["U+10FAA"] } ],
        "compose_sequences": [ { "keys": ["U+10F70","U+10F76"], "output": ["U+10F99"] } ]
    })";
    auto a = LayoutParser::parseString(json);
    CHECK(a.ok());
    std::string out = LayoutSerializer::toJson(*a.layout);
    auto b = LayoutParser::parseString(out);
    CHECK(b.ok());
    if (!b.ok()) return;

    CHECK_EQ(a.layout->meta().id, b.layout->meta().id);
    CHECK_EQ(a.layout->meta().version, b.layout->meta().version);
    CHECK(a.layout->capsMode() == b.layout->capsMode());
    CHECK_EQ(a.layout->normalize(), b.layout->normalize());
    CHECK_EQ(a.layout->keys().size(), b.layout->keys().size());
    CHECK_EQ(a.layout->deadKeys().size(), b.layout->deadKeys().size());
    CHECK_EQ(a.layout->ligatures().size(), b.layout->ligatures().size());
    CHECK_EQ(a.layout->composeSequences().size(), b.layout->composeSequences().size());
    // Deep-check one key and the dead key.
    CHECK(b.layout->key(vk::KeyA)->levels[0].output == std::u32string{ 0x10F70 });
    CHECK(b.layout->key(vk::OEM_2)->levels[2].kind == ActionKind::Compose);
    CHECK(b.layout->deadKey("d1") != nullptr);
}

// ---- Document editing + undo/redo -------------------------------------------

TEST(Document_EditAndUndoRedo)
{
    LayoutDocument doc;
    doc.newLayout("t", "T");
    CHECK(doc.layout().keys().empty());

    doc.setKeyAction(vk::KeyA, Level::Base, emit(U"\U00010F70"));
    CHECK_EQ(doc.layout().keys().size(), size_t(1));
    CHECK(doc.dirty());
    CHECK(doc.canUndo());

    doc.setKeyAction(vk::KeyB, Level::Base, emit(U"\U00010F71"));
    CHECK_EQ(doc.layout().keys().size(), size_t(2));

    CHECK(doc.undo());                       // removes B
    CHECK_EQ(doc.layout().keys().size(), size_t(1));
    CHECK(doc.undo());                       // removes A
    CHECK(doc.layout().keys().empty());
    CHECK_FALSE(doc.undo());                  // nothing left

    CHECK(doc.redo());                        // A back
    CHECK_EQ(doc.layout().keys().size(), size_t(1));
    CHECK(doc.redo());                        // B back
    CHECK_EQ(doc.layout().keys().size(), size_t(2));
    CHECK_FALSE(doc.redo());
}

TEST(Document_NewEditInvalidatesRedo)
{
    LayoutDocument doc;
    doc.newLayout("t", "T");
    doc.setKeyAction(vk::KeyA, Level::Base, emit(U"\U00010F70"));
    doc.undo();
    CHECK(doc.canRedo());
    doc.setKeyAction(vk::KeyC, Level::Base, emit(U"\U00010F72")); // new edit
    CHECK_FALSE(doc.canRedo());               // redo history cleared
}

TEST(Document_ClearKeyAndEmptyLevelsDropKey)
{
    LayoutDocument doc;
    doc.newLayout("t", "T");
    doc.setKeyAction(vk::KeyA, Level::Base, emit(U"\U00010F70"));
    // Setting the only level to None removes the key.
    KeyAction none;
    doc.setKeyAction(vk::KeyA, Level::Base, none);
    CHECK(doc.layout().keys().empty());
}

TEST(Document_CopyPasteKey)
{
    LayoutDocument doc;
    doc.newLayout("t", "T");
    doc.setKeyAction(vk::KeyA, Level::Base, emit(U"\U00010F70"));
    doc.setKeyAction(vk::KeyA, Level::Shift, emit(U"\U00010F71"));

    CHECK(doc.copyKey(vk::KeyA));
    CHECK(doc.hasClipboard());
    CHECK(doc.pasteKey(vk::KeyB));
    CHECK(doc.layout().key(vk::KeyB)->levels[0].output == std::u32string{ 0x10F70 });
    CHECK(doc.layout().key(vk::KeyB)->levels[1].output == std::u32string{ 0x10F71 });

    LayoutDocument empty;
    empty.newLayout("e", "E");
    CHECK_FALSE(empty.pasteKey(vk::KeyA));    // nothing copied
}

TEST(Document_MetadataAndValidation)
{
    LayoutDocument doc;
    doc.newLayout("t", "T");
    // Empty layout is invalid (no keys).
    CHECK_FALSE(doc.validate().ok());
    doc.setKeyAction(vk::KeyA, Level::Base, emit(U"\U00010F70"));
    CHECK(doc.validate().ok());

    LayoutMeta m = doc.layout().meta();
    m.author = "Somebody";
    doc.setMeta(m);
    CHECK_EQ(doc.layout().meta().author, std::string("Somebody"));
    CHECK(doc.undo());
    CHECK(doc.layout().meta().author.empty());
}

TEST(Document_SaveLoadRoundTripViaDocument)
{
    LayoutDocument doc;
    doc.newLayout("save_test", "Save Test");
    doc.setKeyAction(vk::KeyA, Level::Base, emit(U"\U00010F70"));
    std::string json = doc.toJson();

    LayoutDocument doc2;
    std::string err;
    CHECK(doc2.loadFromJson(json, &err));
    CHECK_FALSE(doc2.dirty());
    CHECK_EQ(doc2.layout().meta().id, std::string("save_test"));
    CHECK(doc2.layout().key(vk::KeyA) != nullptr);
}

// ---- Unicode names / search -------------------------------------------------

TEST(UnicodeNames_Lookup)
{
    CHECK_EQ(UnicodeNames::name(0x10F70), std::string("OLD UYGHUR LETTER ALEPH"));
    CHECK_EQ(UnicodeNames::name(0x0041), std::string("LATIN CAPITAL LETTER A"));
    CHECK(UnicodeNames::name(0x3000).empty());     // not in curated table
}

TEST(UnicodeNames_SearchByName)
{
    auto r = UnicodeNames::search("ALEPH");
    CHECK(!r.empty());
    CHECK_EQ(r[0].cp, char32_t(0x10F70));

    auto uy = UnicodeNames::search("old uyghur letter");
    CHECK(uy.size() >= 18);                        // all 18 letters at least
}

TEST(UnicodeNames_SearchByCodepoint)
{
    for (const char* q : { "U+10F70", "0x10F70", "10F70" }) {
        auto r = UnicodeNames::search(q);
        CHECK_EQ(r.size(), size_t(1));
        CHECK_EQ(r[0].cp, char32_t(0x10F70));
    }
    // Unnamed-but-valid scalar still returned so it can be inserted.
    auto r = UnicodeNames::search("U+3000");
    CHECK_EQ(r.size(), size_t(1));
    // Surrogate is not a scalar -> no result.
    CHECK(UnicodeNames::search("U+D800").empty());
}

// ---- Keyboard geometry ------------------------------------------------------

TEST(KeyCaps_AnsiAndIso)
{
    const auto& ansi = KeyCaps::caps(BoardType::ANSI);
    const auto& iso  = KeyCaps::caps(BoardType::ISO);
    CHECK(!ansi.empty());
    CHECK(!iso.empty());
    // ISO has at least one more cap than ANSI (the extra key by Left Shift).
    CHECK(iso.size() >= ansi.size());
    CHECK(KeyCaps::width(BoardType::ANSI) > 10.0);
    CHECK(KeyCaps::height(BoardType::ANSI) >= 5.0);
}

TEST(KeyCaps_EditableKeysHaveVk)
{
    for (const auto& c : KeyCaps::caps(BoardType::ANSI)) {
        if (c.editable) CHECK(c.vk != 0);
    }
    // Every letter A-Z must appear as an editable cap.
    for (unsigned vk = vk::KeyA; vk <= vk::KeyZ; ++vk) {
        bool found = false;
        for (const auto& c : KeyCaps::caps(BoardType::ANSI))
            if (c.editable && c.vk == vk) { found = true; break; }
        CHECK(found);
    }
}
