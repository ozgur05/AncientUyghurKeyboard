cmake_minimum_required(VERSION 3.20)

# Read the canonical version from the top-level VERSION file (shared with the
# installer and CI, so there is one source of truth).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" AUK_VERSION LIMIT_COUNT 1)
string(STRIP "${AUK_VERSION}" AUK_VERSION)

project(AncientUyghurKeyboard VERSION ${AUK_VERSION} LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Language
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AUK_BUILD_TESTS "Build unit tests" ON)

# ---------------------------------------------------------------------------
# core — portable mapping library (Win32-free, so it is unit-testable anywhere)
# ---------------------------------------------------------------------------
add_library(auk_core STATIC
    src/core/KeyboardLayout.cpp
    src/core/LayoutParser.cpp
    src/core/LayoutValidator.cpp
    src/core/LayoutRegistry.cpp
    src/core/KeyboardLayoutManager.cpp
    src/core/Normalizer.cpp
    src/core/Composer.cpp
    src/core/Version.cpp
    src/core/InstallationDetector.cpp
    src/core/Migration.cpp
)
target_include_directories(auk_core PUBLIC src)
target_compile_features(auk_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(auk_core PRIVATE /W4 /permissive- /utf-8)
endif()

# ---------------------------------------------------------------------------
# The Windows application (only when targeting Windows)
# ---------------------------------------------------------------------------
if(WIN32)
    add_compile_definitions(UNICODE _UNICODE)
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    add_executable(AncientUyghurKeyboard WIN32
        src/main.cpp
        src/Application.cpp
        src/KeyboardEngine.cpp
        src/Config.cpp
        src/Logger.cpp
        src/VersionManager.cpp
        src/AutoMigration.cpp
        src/InstallerHelper.cpp
        src/resources.rc
    )
    target_link_libraries(AncientUyghurKeyboard PRIVATE
        auk_core user32 gdi32 shell32 advapi32 ole32 uuid)

    if(MSVC)
        target_compile_options(AncientUyghurKeyboard PRIVATE /W4 /permissive- /utf-8)
        target_link_options(AncientUyghurKeyboard PRIVATE /SUBSYSTEM:WINDOWS)
    endif()

    add_custom_command(TARGET AncientUyghurKeyboard POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/layouts"
                "$<TARGET_FILE_DIR:AncientUyghurKeyboard>/layouts"
        COMMENT "Copying keyboard layouts")
endif()

# ---------------------------------------------------------------------------
# Tests (portable; run on any platform, incl. the CI test job)
# ---------------------------------------------------------------------------
if(AUK_BUILD_TESTS)
    enable_testing()
    add_executable(auk_tests
        tests/test_main.cpp
        tests/test_unicode.cpp
        tests/test_virtualkeys.cpp
        tests/test_normalizer.cpp
        tests/test_layoutparser.cpp
        tests/test_layoutvalidator.cpp
        tests/test_registry.cpp
        tests/test_manager.cpp
        tests/test_composer.cpp
        tests/test_version.cpp
        tests/test_installdetector.cpp
        tests/test_migration.cpp
        tests/test_hardening.cpp
    )
    target_link_libraries(auk_tests PRIVATE auk_core)
    if(MSVC)
        target_compile_options(auk_tests PRIVATE /utf-8)
    endif()
    add_test(NAME unit COMMAND auk_tests)
endif()

# ---------------------------------------------------------------------------
# Benchmark utility (portable; built on demand, not part of the test suite)
# ---------------------------------------------------------------------------
option(AUK_BUILD_BENCH "Build the performance benchmark utility" ON)
if(AUK_BUILD_BENCH)
    add_executable(auk_bench bench/benchmark.cpp)
    target_link_libraries(auk_bench PRIVATE auk_core)
    if(MSVC)
        target_compile_options(auk_bench PRIVATE /O2 /utf-8)
    endif()
endif()
