#include "TestFramework.hpp"
#include "../src/core/Migration.hpp"

using namespace core;

TEST(Migration_PreservesUserValues)
{
    // A user who changed layout/log_level keeps those exact values.
    ConfigMap existing = { {"enabled","0"}, {"layout","old_uyghur_qwerty"},
                           {"log_level","trace"} };
    ConfigMap out = Migration::migrateConfig(existing, Version(0,3,0), Version(0,4,0));
    CHECK_EQ(out["enabled"],   std::string("0"));
    CHECK_EQ(out["layout"],    std::string("old_uyghur_qwerty"));
    CHECK_EQ(out["log_level"], std::string("trace"));
}

TEST(Migration_PreservesUnknownKeys)
{
    // Forward-compatibility: keys we don't recognise must survive migration.
    ConfigMap existing = { {"future_option","42"} };
    ConfigMap out = Migration::migrateConfig(existing, Version(0,3,0), Version(0,4,0));
    CHECK_EQ(out["future_option"], std::string("42"));
}

TEST(Migration_FillsMissingDefaults)
{
    ConfigMap out = Migration::migrateConfig(ConfigMap{}, Version(0,3,0), Version(0,4,0));
    CHECK_EQ(out["enabled"],   std::string("1"));
    CHECK_EQ(out["layout"],    std::string("old_uyghur"));
    CHECK_EQ(out["log_level"], std::string("info"));
}

TEST(Migration_RenamesDeprecatedKey)
{
    // Legacy "enable" becomes "enabled", value preserved.
    ConfigMap existing = { {"enable","0"} };
    ConfigMap out = Migration::migrateConfig(existing, Version(0,3,0), Version(0,4,0));
    CHECK_EQ(out["enabled"], std::string("0"));
    CHECK(out.find("enable") == out.end());
}

TEST(Migration_RenameDoesNotClobberExisting)
{
    // If both old and new keys exist, the modern one wins and old is dropped.
    ConfigMap existing = { {"enable","0"}, {"enabled","1"} };
    ConfigMap out = Migration::migrateConfig(existing, Version(0,3,0), Version(0,4,0));
    CHECK_EQ(out["enabled"], std::string("1"));
    CHECK(out.find("enable") == out.end());
}

TEST(Migration_LayoutSeed_OnlyMissing)
{
    std::vector<std::string> bundled  = { "old_uyghur", "old_uyghur_qwerty" };
    std::vector<std::string> existing = { "old_uyghur" };            // user already has one
    auto plan = Migration::planLayoutSeed(bundled, existing);
    CHECK_EQ(plan.size(), size_t(1));
    CHECK_EQ(plan[0], std::string("old_uyghur_qwerty"));
}

TEST(Migration_LayoutSeed_PreservesUserOnlyLayouts)
{
    // A user-authored layout not in the bundle is never touched, and nothing is
    // re-copied when everything bundled already exists.
    std::vector<std::string> bundled  = { "old_uyghur" };
    std::vector<std::string> existing = { "old_uyghur", "my_custom" };
    auto plan = Migration::planLayoutSeed(bundled, existing);
    CHECK(plan.empty());
}

TEST(Migration_LayoutUpgrade_NewerBundledUpgrades)
{
    // When the bundled version is higher, the layout should be in the upgrade list.
    std::map<std::string, int> bundledVer = { {"old_uyghur", 6}, {"old_uyghur_qwerty", 3} };
    std::map<std::string, int> userVer    = { {"old_uyghur", 4}, {"old_uyghur_qwerty", 1} };
    auto plan = Migration::planLayoutUpgrade(bundledVer, userVer);
    CHECK_EQ(plan.size(), size_t(2));
    CHECK_EQ(plan[0], std::string("old_uyghur"));
    CHECK_EQ(plan[1], std::string("old_uyghur_qwerty"));
}

TEST(Migration_LayoutUpgrade_EqualOrHigherUserPreserved)
{
    // When the user version is equal or higher, no upgrade.
    std::map<std::string, int> bundledVer = { {"old_uyghur", 6}, {"old_uyghur_qwerty", 3} };
    std::map<std::string, int> userVer    = { {"old_uyghur", 6}, {"old_uyghur_qwerty", 7}, {"my_custom", 1} };
    auto plan = Migration::planLayoutUpgrade(bundledVer, userVer);
    CHECK(plan.empty());
}

TEST(Migration_LayoutUpgrade_MissingUserNotUpgraded)
{
    // If the user doesn't have a layout at all, it's not an upgrade (it's a seed).
    std::map<std::string, int> bundledVer = { {"old_uyghur", 6} };
    std::map<std::string, int> userVer    = {};
    auto plan = Migration::planLayoutUpgrade(bundledVer, userVer);
    CHECK(plan.empty());
}
