#include "TestFramework.hpp"
#include "../src/core/Version.hpp"

using namespace core;

TEST(Version_ParseForms)
{
    CHECK(Version::parse("1.2.3").value() == Version(1, 2, 3));
    CHECK(Version::parse("1.2").value()   == Version(1, 2, 0));
    CHECK(Version::parse("4").value()     == Version(4, 0, 0));
    CHECK(Version::parse("v0.4.0").value()== Version(0, 4, 0));
    CHECK(Version::parse("1.2.3-rc1").value() == Version(1, 2, 3));
    CHECK(Version::parse("1.2.3+build7").value() == Version(1, 2, 3));
    CHECK(Version::parse("  2.0.1  ").value() == Version(2, 0, 1));
}

TEST(Version_ParseInvalid)
{
    CHECK_FALSE(Version::parse("").has_value());
    CHECK_FALSE(Version::parse("abc").has_value());
    CHECK_FALSE(Version::parse("1.x.0").has_value());
    CHECK_FALSE(Version::parse("1.2.3.4").has_value());
    CHECK_FALSE(Version::parse("..").has_value());
}

TEST(Version_Compare)
{
    CHECK(Version(1, 0, 0) < Version(1, 0, 1));
    CHECK(Version(1, 2, 0) < Version(1, 10, 0));   // numeric, not lexical
    CHECK(Version(2, 0, 0) > Version(1, 9, 9));
    CHECK(Version(1, 2, 3) == Version(1, 2, 3));
    CHECK(Version(0, 4, 0) >= Version(0, 4, 0));
    CHECK(Version(0, 3, 0) <= Version(0, 4, 0));
}

TEST(Version_RoundTrip)
{
    CHECK_EQ(Version(0, 4, 0).toString(), std::string("0.4.0"));
    CHECK(Version::parse(Version(3, 14, 159).toString()).value() == Version(3, 14, 159));
}
