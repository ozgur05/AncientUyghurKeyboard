<!-- Thanks for contributing! Please fill this in and keep the checklist honest. -->

## Summary

What does this PR do, and why?

Closes #<!-- issue number -->

## Type of change

- [ ] `fix` — bug fix
- [ ] `feat` — new feature
- [ ] `perf` — performance
- [ ] `refactor` — no behavior change
- [ ] `docs` — documentation only
- [ ] `test` / `ci` / `chore`

## Changes

- …

## Checklist

- [ ] Builds with **MSVC** and **GCC/MinGW** (`-std=c++20`), warning-free.
- [ ] `ctest` passes; new/changed core behavior has **tests**.
- [ ] No new external runtime dependencies (native C++20 / Win32 only).
- [ ] `src/core/` changes stay free of `<windows.h>`.
- [ ] All external input is validated; no handle/GDI/memory leaks introduced.
- [ ] No `TODO`/placeholder code left behind.
- [ ] Ran `clang-format`; ran `clang-tidy`/`cppcheck` on non-trivial logic.
- [ ] Updated `CHANGELOG.md` and relevant docs (README / `docs/`) if behavior or
      public API changed.
- [ ] Version bumped in `VERSION` (+ `AppVersion.hpp`) if this is release-bound.

## Testing notes

How did you verify this? (unit tests, manual steps, screenshots for the GUI, …)
