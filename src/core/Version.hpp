// Version.hpp — semantic-ish version (major.minor.patch) parsing + comparison.
//
// Portable and dependency-free so installation/upgrade logic is unit-testable
// without Windows. Trailing components default to 0 ("1" == "1.0.0"); any
// pre-release/build suffix after a '-' or '+' is ignored for comparison.
#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace core {

struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;

    Version() = default;
    Version(int ma, int mi, int pa) : major(ma), minor(mi), patch(pa) {}

    // Parse "1", "1.2", "1.2.3" (with optional "-rc1"/"+meta" suffix). Returns
    // nullopt if the numeric core is malformed.
    static std::optional<Version> parse(const std::string& s);

    std::string toString() const;

    // Ordered comparison by (major, minor, patch).
    int compare(const Version& o) const;
    bool operator==(const Version& o) const { return compare(o) == 0; }
    bool operator!=(const Version& o) const { return compare(o) != 0; }
    bool operator<(const Version& o)  const { return compare(o) <  0; }
    bool operator>(const Version& o)  const { return compare(o) >  0; }
    bool operator<=(const Version& o) const { return compare(o) <= 0; }
    bool operator>=(const Version& o) const { return compare(o) >= 0; }

    bool isZero() const { return major == 0 && minor == 0 && patch == 0; }
};

} // namespace core
