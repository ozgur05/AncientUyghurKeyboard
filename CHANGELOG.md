# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/), and the project follows
[Semantic Versioning](https://semver.org/).

## [1.0.0] — 2026-07-09

First stable release. A 100% offline, native Win32/C++20 keyboard and input
method for the **Old Uyghur** script (Unicode `U+10F70–U+10F89`), with a full
layout system, a graphical layout designer, an installer/release pipeline, and
an optional native TSF IME. No Python/.NET/Java or external runtime.

### Added
- **Mapping core** (`src/core/`, Win32-free, unit-tested): `KeyboardLayout`,
  `LayoutParser`, `LayoutValidator`, `LayoutRegistry`, `KeyboardLayoutManager`,
  `Composer` (dead keys, compose sequences, ligatures, NFC reconciliation),
  `Normalizer`, `Unicode`, `VirtualKeys`.
- **Keyboard hook backend**: global `WH_KEYBOARD_LL` + `SendInput`
  (`KEYEVENTF_UNICODE`), system-tray app with enable toggle and live layout
  switching, hot-reload of layout files.
- **JSON layout format**: four modifier levels (Base/Shift/AltGr/Shift+AltGr),
  dead keys, compose sequences, ligatures, Caps-Lock policy, metadata; two
  bundled layouts (`old_uyghur`, `old_uyghur_qwerty`).
- **Installer & packaging**: Inno Setup installer (per-user/per-machine, silent,
  upgrade-safe), portable ZIP, first-run installation detector + auto-migration,
  version marker, single-instance mutex.
- **Performance & hardening**: O(1) compose matching, allocation-light hot path,
  scalar/surrogate input validation, RAII Win32 wrappers, exception-safe scan,
  `Stopwatch` diagnostics + benchmark utility.
- **Release automation**: GitHub Actions (Debug+Release matrix, `ctest`,
  cppcheck, installer, portable ZIP, source archive, PDB, SHA-256 checksums,
  release notes, optional Authenticode signing), build provenance
  (`BuildInfo.hpp`: version + git hash + build number + timestamp).
- **Layout Designer** (`AncientUyghurDesigner.exe`): visual ANSI/ISO keyboard
  editor with per-level key editing, Unicode picker, live preview, validation
  panel, undo/redo, copy/paste, import/export, backup-before-save.
- **TSF IME** (`AncientUyghurTsf.dll`): native Text Services Framework input
  method as a second, optional backend (the hook remains the default fallback).

### Notes
- The keyboard-hook backend and the entire `core` are compiled and test-verified.
- The Layout Designer GUI and the TSF DLL are compile/link-verified; their
  interactive/registered runtime behavior should be validated on target Windows.

[1.0.0]: https://github.com/ozgur05/AncientUyghurKeyboard/releases/tag/v1.0.0
