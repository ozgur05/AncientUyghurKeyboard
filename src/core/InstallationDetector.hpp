// InstallationDetector.hpp — pure logic to classify how the app was launched.
//
// No OS calls: the caller gathers the raw facts (stored marker version, current
// build version, whether config/data already exist, portable marker) and this
// module decides First-install vs Upgrade vs Downgrade vs Reinstall vs Normal,
// and where the writable data root should be. Kept portable so it is unit-tested.
#pragma once

#include "Version.hpp"
#include <optional>
#include <string>

namespace core {

enum class LaunchKind {
    FirstInstall, // no prior marker and no prior data — fresh
    Upgrade,      // stored version older than current (or legacy data, no marker)
    Downgrade,    // stored version newer than current
    Reinstall,    // stored version equals current
    Normal        // routine launch (marker equals current, data present)
};

struct InstallationStatus {
    LaunchKind         kind = LaunchKind::FirstInstall;
    std::optional<Version> previous;   // marker version, if any
    Version            current;
    bool               configExisted   = false;
    bool               dataDirExisted  = false;

    bool needsMigration() const {
        return kind == LaunchKind::FirstInstall ||
               kind == LaunchKind::Upgrade      ||
               kind == LaunchKind::Reinstall;   // reinstall may need dir/layout repair
    }
    bool isUpgrade()      const { return kind == LaunchKind::Upgrade; }
    bool isFirstInstall() const { return kind == LaunchKind::FirstInstall; }
};

class InstallationDetector {
public:
    // Classify the launch from gathered facts.
    static InstallationStatus detect(std::optional<Version> stored,
                                     Version current,
                                     bool configExisted,
                                     bool dataDirExisted);

    // Decide the writable data root: the executable directory in portable mode,
    // otherwise the per-user application-data directory.
    static std::string chooseDataRoot(bool portableMarkerPresent,
                                      const std::string& exeDir,
                                      const std::string& appDataDir);
};

} // namespace core
