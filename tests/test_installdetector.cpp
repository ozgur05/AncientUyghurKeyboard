#include "TestFramework.hpp"
#include "../src/core/InstallationDetector.hpp"

using namespace core;

TEST(Detect_FirstInstall)
{
    // No marker, no config, no data dir -> fresh install.
    auto s = InstallationDetector::detect(std::nullopt, Version(0, 4, 0), false, false);
    CHECK(s.kind == LaunchKind::FirstInstall);
    CHECK(s.isFirstInstall());
    CHECK(s.needsMigration());
}

TEST(Detect_LegacyNoMarkerIsUpgrade)
{
    // No marker but pre-existing config -> legacy install, migrate as upgrade.
    auto s = InstallationDetector::detect(std::nullopt, Version(0, 4, 0), true, true);
    CHECK(s.kind == LaunchKind::Upgrade);
    CHECK(s.needsMigration());
}

TEST(Detect_Upgrade)
{
    auto s = InstallationDetector::detect(Version(0, 3, 0), Version(0, 4, 0), true, true);
    CHECK(s.kind == LaunchKind::Upgrade);
    CHECK(s.isUpgrade());
    CHECK(s.previous.value() == Version(0, 3, 0));
}

TEST(Detect_Downgrade)
{
    auto s = InstallationDetector::detect(Version(0, 5, 0), Version(0, 4, 0), true, true);
    CHECK(s.kind == LaunchKind::Downgrade);
    CHECK_FALSE(s.needsMigration());
}

TEST(Detect_NormalLaunch)
{
    auto s = InstallationDetector::detect(Version(0, 4, 0), Version(0, 4, 0), true, true);
    CHECK(s.kind == LaunchKind::Normal);
    CHECK_FALSE(s.needsMigration());
}

TEST(Detect_ReinstallWhenDataMissing)
{
    // Marker equals current but data was deleted -> repair (reinstall).
    auto s = InstallationDetector::detect(Version(0, 4, 0), Version(0, 4, 0), false, false);
    CHECK(s.kind == LaunchKind::Reinstall);
    CHECK(s.needsMigration());
}

TEST(Detect_ChooseDataRoot)
{
    CHECK_EQ(InstallationDetector::chooseDataRoot(true,  "C:\\App", "C:\\Users\\x\\AppData"),
             std::string("C:\\App"));
    CHECK_EQ(InstallationDetector::chooseDataRoot(false, "C:\\App", "C:\\Users\\x\\AppData"),
             std::string("C:\\Users\\x\\AppData"));
}
