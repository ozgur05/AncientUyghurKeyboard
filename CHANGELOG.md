# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/), and the project follows
[Semantic Versioning](https://semver.org/).

## [1.1.0] — 2026-11-24

### Added
- **Text-shaping controls on AltGr layer**: ZWJ (`AltGr+=`), ZWNJ (`AltGr+-`),
  Arabic Tatweel/kashida (`AltGr+/`), and Manichaean punctuation (U+10AF2) via
  compose sequence — essential for controlling OpenType positional forms
  (`.init`, `.medi`, `.fina`) and ligatures in Old Uyghur fonts.
- **5 ligature compose shortcuts** (via `AltGr+/` then two keys): Kaph+Lesh,
  Kaph+Waw, Mem+Lesh, Pe+Lesh, Pe+Waw — triggers the font's `rlig`/`ccmp`
  ligature glyphs.
- **`dots_below` dead key** on `Shift+backtick` — mirrors the existing
  `dots_above` dead key, adding TWO DOTS BELOW (U+10F85) to any letter.
- **Expanded `dots_above` dead key** to cover all 18 Old Uyghur letters
  (previously only Aleph, Yodh, Nun).
- Quick-access combining marks on `,` (DOT ABOVE) and `.` (TWO DOTS ABOVE).

### Changed
- Phonetic layout (`old_uyghur.json`) bumped from v4 to v5.
- QWERTY layout (`old_uyghur_qwerty.json`) bumped from v1 to v2.
- Both layouts now expose all typable codepoints in QochoOldUyghur-Regular
  v1.000 (26 Old Uyghur + 4 shaping controls + Manichaean punctuation).

### Notes
- The 109 glyphs in QochoOldUyghur-Regular include 50 positional forms and 22
  ligatures that are automatically selected by OpenType shaping (`init`, `medi`,
  `fina`, `rlig`, `ccmp` features) — they do not need separate key mappings.
- For best rendering, use LibreOffice Writer or a browser (Firefox/Chrome) which
  have full HarfBuzz OpenType shaping support for SMP scripts.

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

[1.1.0]: https://github.com/ozgur05/AncientUyghurKeyboard/releases/tag/v1.1.0
[1.0.0]: https://github.com/ozgur05/AncientUyghurKeyboard/releases/tag/v1.0.0
