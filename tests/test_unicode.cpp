#include "TestFramework.hpp"
#include "../src/core/Unicode.hpp"

using namespace core::unicode;

TEST(Unicode_Utf16_Bmp)
{
    char16_t units[2];
    CHECK_EQ(toUtf16(U'A', units), 1);
    CHECK_EQ(units[0], u'A');
}

TEST(Unicode_Utf16_Supplementary)
{
    // U+10F70 (Old Uyghur Aleph) must become a surrogate pair.
    char16_t units[2];
    CHECK_EQ(toUtf16(0x10F70, units), 2);
    CHECK_EQ(units[0], char16_t(0xD803));
    CHECK_EQ(units[1], char16_t(0xDF70));
    CHECK_EQ(utf16Length(0x10F70), 2);
    CHECK_EQ(utf16Length(U'A'), 1);
}

TEST(Unicode_RoundTrip_Utf8)
{
    std::u32string src = { 0x41, 0x10F70, 0x00E4, 0x10F84 };
    std::string utf8 = utf32ToUtf8(src);
    std::u32string back = utf8ToUtf32(utf8);
    CHECK(src == back);
}

TEST(Unicode_RoundTrip_Utf16)
{
    std::u32string src = { 0x10F70, 0x10F71, 0x0062, 0x10FFFF };
    std::u16string u16 = utf32ToUtf16(src);
    std::u32string back = utf16ToUtf32(u16);
    CHECK(src == back);
}

TEST(Unicode_Invalid_ToReplacement)
{
    // Lone surrogate decoded from UTF-8 becomes U+FFFD.
    std::string bad = "\xED\xA0\x80"; // encodes D800 (illegal in UTF-8)
    std::u32string out = utf8ToUtf32(bad);
    CHECK_EQ(out.size(), size_t(1));
    CHECK_EQ(out[0], kReplacement);
}
