#include "TestFramework.hpp"
#include "../src/core/VirtualKeys.hpp"

using namespace core;

TEST(VK_Letters)
{
    CHECK_EQ(vk::fromName("A").value_or(0), unsigned(vk::KeyA));
    CHECK_EQ(vk::fromName("z").value_or(0), unsigned(vk::KeyZ));
    CHECK(vk::isLetter(vk::KeyA));
    CHECK_FALSE(vk::isLetter(vk::Space));
}

TEST(VK_Digits)
{
    CHECK_EQ(vk::fromName("0").value_or(0), unsigned(vk::Key0));
    CHECK_EQ(vk::fromName("9").value_or(0), unsigned(vk::Key9));
}

TEST(VK_Named)
{
    CHECK_EQ(vk::fromName("Space").value_or(0),     unsigned(vk::Space));
    CHECK_EQ(vk::fromName("Enter").value_or(0),     unsigned(vk::Enter));
    CHECK_EQ(vk::fromName("Return").value_or(0),    unsigned(vk::Enter));
    CHECK_EQ(vk::fromName("Backspace").value_or(0), unsigned(vk::Back));
    CHECK_EQ(vk::fromName("Tab").value_or(0),       unsigned(vk::Tab));
}

TEST(VK_Oem_And_Aliases)
{
    CHECK_EQ(vk::fromName("OEM_1").value_or(0),     unsigned(vk::OEM_1));
    CHECK_EQ(vk::fromName("semicolon").value_or(0), unsigned(vk::OEM_1));
    CHECK_EQ(vk::fromName("LBracket").value_or(0),  unsigned(vk::OEM_4));
}

TEST(VK_Numeric)
{
    CHECK_EQ(vk::fromName("0xBA").value_or(0), unsigned(0xBA));
    CHECK_EQ(vk::fromName("32").value_or(0),   unsigned(32));
}

TEST(VK_Unknown)
{
    CHECK_FALSE(vk::fromName("NotAKey").has_value());
    CHECK_FALSE(vk::fromName("").has_value());
}
