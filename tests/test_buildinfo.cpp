#include "TestFramework.hpp"
#include "../src/BuildInfo.hpp"
#include "../src/core/Version.hpp"

#include <string>

TEST(BuildInfo_VersionMatchesAppVersion)
{
    // The embedded version must be the same string AppVersion.hpp declares and
    // must parse as a valid semantic version.
    CHECK_EQ(std::string(buildinfo::version()), std::string(AUK_APP_VERSION_STR));
    auto v = core::Version::parse(buildinfo::version());
    CHECK(v.has_value());
}

TEST(BuildInfo_FieldsPresent)
{
    CHECK(std::string(buildinfo::gitHash()).size() > 0);
    CHECK(std::string(buildinfo::buildNumber()).size() > 0);
    CHECK(std::string(buildinfo::timestamp()).size() > 0);
}

TEST(BuildInfo_FullContainsParts)
{
    std::string full = buildinfo::full();
    CHECK(full.find(buildinfo::version()) != std::string::npos);
    CHECK(full.find(buildinfo::gitHash()) != std::string::npos);
    CHECK(full.find("build.") != std::string::npos);
}
