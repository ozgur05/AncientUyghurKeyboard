// Migration.hpp — pure, testable migration transforms.
//
// These functions never touch the filesystem; they compute *what* should
// change. The Win32 AutoMigration class applies the results. Keeping them pure
// lets the tests prove configuration- and layout-preservation guarantees.
#pragma once

#include "Version.hpp"
#include <map>
#include <string>
#include <vector>

namespace core {

// Ordered map keeps config output deterministic (nice diffs, stable tests).
using ConfigMap = std::map<std::string, std::string>;

class Migration {
public:
    // Transform an existing config into the current schema:
    //   * every user-set key is preserved (unknown keys included),
    //   * deprecated keys are renamed to their modern equivalents,
    //   * missing required keys are filled with defaults.
    // `from`/`to` are available for version-specific rules; the current schema
    // has one rename rule (pre-0.4 "enable" -> "enabled").
    static ConfigMap migrateConfig(const ConfigMap& existing,
                                   const Version& from, const Version& to);

    // Default values for required keys (used to fill gaps, never to overwrite).
    static ConfigMap defaults();

    // Given the bundled layout ids and the ids already present in the user
    // layouts directory, return the bundled ids that must be copied in. Existing
    // user layouts are never listed, so user edits are preserved.
    static std::vector<std::string> planLayoutSeed(
        const std::vector<std::string>& bundled,
        const std::vector<std::string>& existing);
};

} // namespace core
