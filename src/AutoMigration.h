// AutoMigration.h — applies migration to the filesystem at startup.
//
// Uses the pure core::Migration plans to perform the side effects:
//   * recreate missing data directories (data root + layouts subdir),
//   * seed bundled layouts that are absent from the user layouts dir
//     (never overwriting a user's file),
//   * migrate config.ini in place (preserving user + unknown keys).
// All operations are idempotent and safe to run on every launch.
#pragma once

#include <string>
#include "core/InstallationDetector.hpp"

class AutoMigration {
public:
    struct Paths {
        std::wstring dataRoot;        // resolved writable root
        std::wstring userLayoutsDir;  // dataRoot\layouts
        std::wstring bundledLayoutsDir; // ExeDir\layouts (read-only source)
        std::wstring configPath;      // dataRoot\config.ini
    };

    // Perform migration appropriate to `status`. Returns the number of actions
    // taken (dirs created + layouts copied + config rewritten); 0 means nothing
    // needed doing. Failures are logged via Logger and skipped, never fatal.
    static int Run(const core::InstallationStatus& status, const Paths& paths);

    // Recreate the data root and layouts directory if missing. Returns count.
    static int EnsureDirectories(const Paths& paths);

    // Copy bundled layouts missing from the user dir. Returns count copied.
    static int SeedLayouts(const Paths& paths);

    // Load, migrate, and rewrite config.ini. Returns 1 if rewritten, else 0.
    static int MigrateConfigFile(const core::InstallationStatus& status,
                                 const Paths& paths);
};
