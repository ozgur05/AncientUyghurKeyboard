# Contributing

Thanks for your interest in the Ancient Uyghur Keyboard. This project is a
native Win32 / C++20 application with **no external runtime dependencies**;
please keep contributions within those constraints.

## Ground rules

- **C++20, Win32 API only.** No Python, .NET, Java, or third-party runtime
  libraries. Standard-library and Windows SDK only.
- **Keep the core portable.** Everything under `src/core/` must compile without
  `<windows.h>` so it can be unit-tested on any platform. OS-specific code lives
  in the app / designer / TSF layers.
- **Tests are mandatory** for core changes. Add cases under `tests/` and make
  sure `ctest` is green.
- **No placeholder code, no `TODO` comments** in committed work — finish the
  change or open an issue describing what remains.

## Development setup

See [`docs/DEVELOPER_GUIDE.md`](docs/DEVELOPER_GUIDE.md) for building with MSVC
or MinGW, running the tests, and building the installer/packages. Quick start:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The portable core + tests also build with GCC/Clang:

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

## Coding standards

- 4-space indentation, 100-column soft limit, `.clang-format` is authoritative
  (`clang-format -i` before submitting).
- **Naming:** `PascalCase` for types and Win32-layer methods, `camelCase` for
  core methods and locals, `m_` prefix for members, `k`-prefix for constants,
  `UPPER_SNAKE` for macros.
- Prefer RAII for any Win32 handle (`src/win/ScopedResources.h`); never leak
  handles or GDI objects.
- Validate all external input (JSON, codepoints, file paths). Codepoints must be
  valid Unicode scalar values — reject surrogates and out-of-range values.
- Run `.clang-tidy` and `cppcheck` locally when touching non-trivial logic.

## Commit conventions

Conventional Commits: `type(scope): summary`, e.g.
`feat(layout): add Shift+AltGr level`, `fix(composer): correct NFC ordering`.
Types used here: `feat`, `fix`, `perf`, `refactor`, `docs`, `test`, `ci`,
`chore`. End commit bodies with a `Co-Authored-By:` line when applicable.

## Branch & PR workflow

- `main` is always releasable; CI must be green.
- Work on a topic branch (`feat/…`, `fix/…`) and open a PR against `main`.
- Fill in the pull-request template; link the issue it closes.
- A PR must build (MSVC **and** GCC), pass all tests, and update docs/CHANGELOG
  when it changes behavior or public API.

## Reporting bugs / requesting features

Use the issue templates (`.github/ISSUE_TEMPLATE/`). For security issues, follow
[`SECURITY.md`](SECURITY.md) instead of opening a public issue.

By contributing you agree your work is licensed under the project's
[MIT License](LICENSE) and that you follow the
[Code of Conduct](CODE_OF_CONDUCT.md).
