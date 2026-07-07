#include "InstallationDetector.hpp"

namespace core {

InstallationStatus InstallationDetector::detect(std::optional<Version> stored,
                                                Version current,
                                                bool configExisted,
                                                bool dataDirExisted)
{
    InstallationStatus s;
    s.previous       = stored;
    s.current        = current;
    s.configExisted  = configExisted;
    s.dataDirExisted = dataDirExisted;

    if (!stored) {
        // No version marker. If there is pre-existing config/data, this is an
        // upgrade from a marker-less (legacy) install that must be migrated;
        // otherwise it is a genuine first install.
        s.kind = (configExisted || dataDirExisted) ? LaunchKind::Upgrade
                                                    : LaunchKind::FirstInstall;
        return s;
    }

    const int cmp = stored->compare(current);
    if (cmp < 0)      s.kind = LaunchKind::Upgrade;
    else if (cmp > 0) s.kind = LaunchKind::Downgrade;
    else {
        // Marker equals current: routine launch, unless data is missing (the
        // user deleted it / a repair is needed), in which case treat as reinstall.
        s.kind = (configExisted && dataDirExisted) ? LaunchKind::Normal
                                                   : LaunchKind::Reinstall;
    }
    return s;
}

std::string InstallationDetector::chooseDataRoot(bool portableMarkerPresent,
                                                 const std::string& exeDir,
                                                 const std::string& appDataDir)
{
    return portableMarkerPresent ? exeDir : appDataDir;
}

} // namespace core
