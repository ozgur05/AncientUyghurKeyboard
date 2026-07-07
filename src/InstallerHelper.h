// InstallerHelper.h — startup facade for installation/upgrade/migration.
//
// Application calls Startup() once, very early. It resolves the writable data
// root (portable vs %APPDATA%), classifies the launch, runs auto-migration, and
// records the current version marker. The returned Result tells Application
// where to read config/logs/layouts and what to report to the user.
#pragma once

#include <string>
#include "core/InstallationDetector.hpp"

class InstallerHelper {
public:
    struct Result {
        core::InstallationStatus status;
        std::wstring dataRoot;       // where config.ini / app.log live
        std::wstring layoutsDir;     // dataRoot\layouts (scanned by the manager)
        bool         portable = false;
    };

    // Resolve paths, detect the launch kind, migrate, and stamp the version
    // marker. Safe to call before Config/Logger are initialised.
    static Result Startup();

    // True when a "portable.ini" marker sits next to the executable.
    static bool PortableMarkerPresent();

    // Directory containing the running executable.
    static std::wstring ExeDir();

    // Per-user application-data directory (%APPDATA%\AncientUyghurKeyboard).
    static std::wstring AppDataDir();
};
