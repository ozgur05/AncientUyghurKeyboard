# Developer Guide

How to build, test, debug, and package the Ancient Uyghur Keyboard.

## Prerequisites

- **Windows 10 or 11** for the full build (the app, designer, TSF DLL, installer).
- **Visual Studio 2022** (MSVC toolset, "Desktop development with C++") — the
  primary/CI toolchain — **or** MinGW-w64 (GCC 13+).
- **CMake ≥ 3.20**.
- Optional: **Inno Setup 6** (`choco install innosetup`) for the installer;
  **cppcheck** / **clang-format** / **clang-tidy** for the quality tooling.

The portable core (`src/core/`) + tests build on any platform, so you can run
the test suite on Linux/macOS with GCC or Clang.

## Project structure

```
src/core/     Portable, unit-tested mapping engine (auk_core static lib)
src/          Keyboard-hook tray app (AncientUyghurKeyboard.exe)
src/designer/ Layout Designer GUI (AncientUyghurDesigner.exe)
src/tsf/      TSF text-service COM DLL (AncientUyghurTsf.dll)
src/win/      Shared RAII wrappers for Win32 resources
tests/        Unit tests (header-only harness) + ctest target
bench/        Micro-benchmarks (auk_bench)
layouts/      Bundled JSON layouts
installer/    Inno Setup script + portable marker
docs/         This documentation set
.github/      CI workflow + issue/PR templates
VERSION       Single source of truth for the version
```

## Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Outputs land in `build\Release\`: `AncientUyghurKeyboard.exe`,
`AncientUyghurDesigner.exe`, `AncientUyghurTsf.dll`, plus the copied `layouts\`.

Useful CMake options: `-DAUK_BUILD_TESTS=ON/OFF`, `-DAUK_BUILD_BENCH=ON/OFF`,
`-DAUK_WERROR=ON` (treat warnings as errors).

Portable/core-only (any platform):

```bash
cmake -S . -B build && cmake --build build
```

## Test

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Tests are plain C++ using the header-only harness in `tests/TestFramework.hpp`
(`TEST(name){ CHECK(...); }`). Add a `test_*.cpp`, list it in `CMakeLists.txt`
under `auk_tests`, and keep the suite green. New core behavior **must** ship with
tests.

## Benchmark

```powershell
build\Release\auk_bench.exe    # times UTF-16 conversion, compose lookup, parsing
```

## Debugging

- **Logs.** Every launch writes `%APPDATA%\AncientUyghurKeyboard\app.log`,
  starting with `Build: <version>+build.N (git …, built …)`. Set
  `log_level=trace` in `config.ini` for verbose output.
- **Hook latency.** Set `AUK_DIAG=1` to log per-keystroke hook timing (trace).
- **Portable mode.** Drop a `portable.ini` next to the exe to keep config/log/
  layouts in the app folder — handy for isolated testing.
- **Designer.** Launch `AncientUyghurDesigner.exe` directly; it opens/saves the
  same layout JSON the app reads. The validation panel shows parser/validator
  output live.
- **TSF DLL.** Register with `regsvr32 AncientUyghurTsf.dll` (elevated),
  select *Ancient Uyghur (TSF)* from the language bar, and test in a TSF-aware
  app (WordPad, Notepad on Win11, browsers). Unregister with `regsvr32 /u`.
  Attach the debugger to the host process (e.g. `notepad.exe`) to debug it.

## Coding standards

- 4-space indent, 100-col soft limit; `.clang-format` is authoritative.
- **Naming:** types & Win32-layer methods `PascalCase`; core methods & locals
  `camelCase`; members `m_`; constants `k`; macros `UPPER_SNAKE`.
- Keep `src/core/` free of `<windows.h>`.
- RAII for every Win32 handle (`src/win/ScopedResources.h`); no GDI/handle leaks.
- Validate all external input; codepoints must be valid Unicode scalar values.
- Run `clang-tidy`/`cppcheck` on non-trivial changes.

## Commit & branch conventions

- **Conventional Commits**: `type(scope): summary` (`feat`, `fix`, `perf`,
  `refactor`, `docs`, `test`, `ci`, `chore`).
- `main` stays releasable and green; work on `feat/…` or `fix/…` branches and
  open a PR using the template.

## Packaging a release

1. Bump `VERSION` (and keep `src/AppVersion.hpp` in sync) and update `CHANGELOG.md`.
2. Push; CI builds Debug+Release, tests, and packages artifacts.
3. Tag `vX.Y.Z` and push the tag — CI publishes a GitHub Release with the
   installer, portable ZIP, source archive, and `SHA256SUMS.txt`.

See [`CI_AND_PUSH.md`](CI_AND_PUSH.md) and [`ARCHITECTURE.md`](ARCHITECTURE.md)
for more.
