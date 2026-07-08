// AppVersion.hpp — single source of the application version for C++ and the .rc.
//
// Keep the numeric triple and the string in sync with the top-level VERSION
// file (CI reads VERSION for installer / ZIP naming). Both are plain values so
// there is no code generation step.
//
// NOTE: the version string is written out literally rather than built with the
// preprocessor stringizing operator (#). Microsoft's resource compiler
// (rc.exe), which processes resources.rc, does not reliably support the `#`
// or `##` operators — using them here would break the MSVC resource build.
#pragma once

#define AUK_VER_MAJOR 0
#define AUK_VER_MINOR 4
#define AUK_VER_PATCH 0

// String form, e.g. "0.4.0". Must match the numeric triple above.
#define AUK_APP_VERSION_STR "0.4.0"
