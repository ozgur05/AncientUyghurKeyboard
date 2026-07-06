#include "TestFramework.hpp"
#include "../src/core/Normalizer.hpp"

using namespace core::normalizer;

TEST(Normalizer_CombiningClass)
{
    CHECK_EQ(combiningClass(U'a'), 0);
    CHECK_EQ(combiningClass(0x0301), 230); // acute
    CHECK_EQ(combiningClass(0x0323), 220); // dot below
    CHECK_EQ(combiningClass(0x10F82), 230); // Old Uyghur dot above
    CHECK_EQ(combiningClass(0x10F83), 220); // Old Uyghur dot below
}

TEST(Normalizer_CanonicalOrder)
{
    // Above(230) then Below(220) must reorder to Below, Above (ascending ccc).
    std::u32string in  = { 0x0061, 0x0301, 0x0323 }; // a + acute(230) + dotbelow(220)
    std::u32string out = canonicalOrder(in);
    std::u32string exp = { 0x0061, 0x0323, 0x0301 };
    CHECK(out == exp);
}

TEST(Normalizer_Compose_Latin)
{
    std::u32string in = { 0x0061, 0x0308 };   // a + diaeresis
    CHECK(toNFC(in) == std::u32string{ 0x00E4 }); // ä
}

TEST(Normalizer_OldUyghur_MarksStayDecomposed)
{
    // No precomposed form exists; NFC must keep letter + mark, correctly ordered.
    std::u32string in  = { 0x10F70, 0x10F84 }; // Aleph + two dots above
    std::u32string out = toNFC(in);
    CHECK(out == in);
}

TEST(Normalizer_OldUyghur_ReorderMarks)
{
    // above(230) below(220) -> below, above
    std::u32string in  = { 0x10F70, 0x10F82, 0x10F83 };
    std::u32string exp = { 0x10F70, 0x10F83, 0x10F82 };
    CHECK(toNFC(in) == exp);
}
