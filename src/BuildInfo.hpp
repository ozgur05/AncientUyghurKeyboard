// BuildInfo.hpp — build provenance embedded at compile time.
//
// The version comes from AppVersion.hpp. The git commit hash and build number
// are injected by CMake as preprocessor definitions; when they are absent
// (e.g. a bare `g++` compile with no CMake), safe fallbacks keep this header
// compiling everywhere. The build timestamp uses the standard __DATE__/__TIME__
// macros, so no code generation step is required.
#pragma once

#include "AppVersion.hpp"
#include <string>

#ifndef AUK_GIT_HASH
#define AUK_GIT_HASH "unknown"
#endif

#ifndef AUK_BUILD_NUMBER
#define AUK_BUILD_NUMBER "0"
#endif

#define AUK_BUILD_TIMESTAMP (__DATE__ " " __TIME__)

namespace buildinfo {

inline const char* version()     { return AUK_APP_VERSION_STR; }
inline const char* gitHash()     { return AUK_GIT_HASH; }
inline const char* buildNumber() { return AUK_BUILD_NUMBER; }
inline const char* timestamp()   { return AUK_BUILD_TIMESTAMP; }

// Human-readable one-liner, e.g.
//   "0.4.0+build.42 (git a1b2c3d4e5f6, built Jul  7 2026 12:00:00)"
inline std::string full()
{
    std::string s = version();
    s += "+build.";
    s += buildNumber();
    s += " (git ";
    s += gitHash();
    s += ", built ";
    s += timestamp();
    s += ")";
    return s;
}

} // namespace buildinfo
