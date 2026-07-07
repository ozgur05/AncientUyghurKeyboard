// AppVersion.hpp — single source of the application version for C++ and the .rc.
//
// Keep this in sync with the top-level VERSION file (used by CI for installer /
// ZIP naming). Both are plain text so no code generation step is required.
#pragma once

#define AUK_VER_MAJOR 0
#define AUK_VER_MINOR 4
#define AUK_VER_PATCH 0

// String form, e.g. "0.4.0". (Stringize via two levels so the macros expand.)
#define AUK_STR2(x) #x
#define AUK_STR(x)  AUK_STR2(x)
#define AUK_APP_VERSION_STR \
    AUK_STR(AUK_VER_MAJOR) "." AUK_STR(AUK_VER_MINOR) "." AUK_STR(AUK_VER_PATCH)
