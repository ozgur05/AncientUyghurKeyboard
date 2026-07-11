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

# Use the static CRT (/MT) on MSVC so the EXE needs no VC++ redistributable.
# This MUST be set before ANY target is created so every target (auk_core, the
# executables, tests, bench) uses the SAME CRT — mixing /MD and /MT otherwise
# causes LNK2005 (multiply defined CRT symbols) when linking auk_core into them.
# Honored via policy CMP0091 (NEW since CMake 3.15).
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(AUK_BUILD_TESTS "Build unit tests" ON)
option(AUK_BUILD_BENCH "Build the performance benchmark utility" ON)
option(AUK_WERROR      "Treat compiler warnings as errors" OFF)

# ---------------------------------------------------------------------------
# Build provenance: git short hash + build number embedded via BuildInfo.hpp.
# ---------------------------------------------------------------------------
find_package(Git QUIET)
set(AUK_GIT_HASH "unknown")
if(Git_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE AUK_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(AUK_GIT_HASH STREQUAL "")
        set(AUK_GIT_HASH "unknown")
    endif()
endif()

set(AUK_BUILD_NUMBER "$ENV{GITHUB_RUN_NUMBER}")
if(AUK_BUILD_NUMBER STREQUAL "")
    set(AUK_BUILD_NUMBER "0")
endif()
message(STATUS "AncientUyghurKeyboard ${AUK_VERSION} (git ${AUK_GIT_HASH}, build ${AUK_BUILD_NUMBER})")

# Apply the shared warning policy (and optional warnings-as-errors) to a target.
function(auk_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(AUK_WERROR)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(AUK_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# Inject build provenance definitions into a target.
function(auk_buildinfo target)
    target_compile_definitions(${target} PRIVATE
        AUK_GIT_HASH="${AUK_GIT_HASH}"
        AUK_BUILD_NUMBER="${AUK_BUILD_NUMBER}")
endfunction()

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
    src/core/LayoutSerializer.cpp
    src/core/LayoutDocument.cpp
    src/core/UnicodeNames.cpp
    src/core/KeyCaps.cpp
)
target_include_directories(auk_core PUBLIC src)
target_compile_features(auk_core PUBLIC cxx_std_20)
auk_warnings(auk_core)

# ---------------------------------------------------------------------------
# The Windows application (only when targeting Windows)
# ---------------------------------------------------------------------------
if(WIN32)
    add_compile_definitions(UNICODE _UNICODE)
    # (CRT runtime library is set once at the top, before any target.)

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
    target_include_directories(AncientUyghurKeyboard PRIVATE src) # for rc.exe includes
    target_link_libraries(AncientUyghurKeyboard PRIVATE
        auk_core user32 gdi32 shell32 advapi32 ole32 uuid)
    auk_warnings(AncientUyghurKeyboard)
    auk_buildinfo(AncientUyghurKeyboard)

    if(MSVC)
        target_link_options(AncientUyghurKeyboard PRIVATE /SUBSYSTEM:WINDOWS)
        # Emit a standalone PDB for Release so debug symbols can be shipped.
        target_compile_options(AncientUyghurKeyboard PRIVATE $<$<CONFIG:Release>:/Zi>)
        target_link_options(AncientUyghurKeyboard PRIVATE
            $<$<CONFIG:Release>:/DEBUG> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
    elseif(MINGW)
        # The app uses wWinMain; MinGW selects the wide entry point via -municode.
        target_link_options(AncientUyghurKeyboard PRIVATE -municode)
    endif()

    add_custom_command(TARGET AncientUyghurKeyboard POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/layouts"
                "$<TARGET_FILE_DIR:AncientUyghurKeyboard>/layouts"
        COMMENT "Copying keyboard layouts")

    # ----- Graphical layout designer (separate GUI executable) -----
    add_executable(AncientUyghurDesigner WIN32
        src/designer/main.cpp
        src/designer/DesignerWindow.cpp
        src/designer/KeyEditDialog.cpp
        src/designer/UnicodePicker.cpp
        src/designer/designer.rc
    )
    target_include_directories(AncientUyghurDesigner PRIVATE src src/designer)
    target_link_libraries(AncientUyghurDesigner PRIVATE
        auk_core comctl32 comdlg32 shell32 ole32 uuid gdi32 user32)
    auk_warnings(AncientUyghurDesigner)
    auk_buildinfo(AncientUyghurDesigner)
    if(MSVC)
        target_link_options(AncientUyghurDesigner PRIVATE /SUBSYSTEM:WINDOWS)
    elseif(MINGW)
        target_link_options(AncientUyghurDesigner PRIVATE -municode)
    endif()

    # Put the designer next to the keyboard app so the tray "Layout Designer…"
    # item can launch it from the same folder.
    add_custom_command(TARGET AncientUyghurDesigner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:AncientUyghurDesigner>"
                "$<TARGET_FILE_DIR:AncientUyghurKeyboard>/AncientUyghurDesigner.exe"
        COMMENT "Placing designer beside the keyboard app")

    # ----- TSF text service (COM in-proc server DLL, second input backend) -----
    # The keyboard hook remains the default; this is an optional native IME.
    add_library(AncientUyghurTsf SHARED
        src/tsf/Guids.cpp
        src/tsf/Dll.cpp
        src/tsf/ClassFactory.cpp
        src/tsf/TextService.cpp
        src/tsf/EditSession.cpp
        src/tsf/Register.cpp
        src/tsf/tsf.rc
        src/tsf/tsf.def
    )
    target_include_directories(AncientUyghurTsf PRIVATE src src/tsf)
    target_link_libraries(AncientUyghurTsf PRIVATE
        auk_core ole32 oleaut32 advapi32 shell32 user32 uuid)
    auk_buildinfo(AncientUyghurTsf)
    # NOTE: the .def provides the COM exports; do not also apply /WX here — the
    # TSF interface surface is verified to compile under MinGW and MSVC.
    if(MSVC)
        target_compile_options(AncientUyghurTsf PRIVATE /W4 /permissive- /utf-8)
    endif()
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
        tests/test_buildinfo.cpp
        tests/test_designer.cpp
    )
    target_link_libraries(auk_tests PRIVATE auk_core)
    target_include_directories(auk_tests PRIVATE src)
    auk_buildinfo(auk_tests)
    if(MSVC)
        target_compile_options(auk_tests PRIVATE /utf-8)
    endif()
    add_test(NAME unit COMMAND auk_tests)
endif()

# ---------------------------------------------------------------------------
# Benchmark utility (portable; built on demand, not part of the test suite)
# ---------------------------------------------------------------------------
if(AUK_BUILD_BENCH)
    add_executable(auk_bench bench/benchmark.cpp)
    target_link_libraries(auk_bench PRIVATE auk_core)
    if(MSVC)
        target_compile_options(auk_bench PRIVATE /O2 /utf-8)
    endif()
endif()
