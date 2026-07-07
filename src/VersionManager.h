// VersionManager.h — reads/writes the installed-version marker file.
//
// The marker (installed_version.txt in the data root) records which build last
// ran, so the InstallationDetector can tell first-install / upgrade / downgrade
// apart. Wraps core::Version for parsing and comparison.
#pragma once

#include <string>
#include <optional>
#include "core/Version.hpp"

class VersionManager {
public:
    // The version compiled into this build (from AppVersion.hpp).
    static core::Version Current();

    // Read the marker at the given path. nullopt if absent or unparceable.
    static std::optional<core::Version> ReadMarker(const std::wstring& path);

    // Write `v` to the marker file. Returns false on I/O failure.
    static bool WriteMarker(const std::wstring& path, const core::Version& v);
};
