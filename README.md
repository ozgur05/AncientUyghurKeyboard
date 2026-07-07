# Ancient Uyghur Keyboard

A tiny, **100% offline** Windows tray application that lets you type in the
**Old Uyghur** script (Unicode block `U+10F70–U+10FAF`) in any application.

- Native Win32, **modern C++20**, **Win32 API only**
- **No** Python / Java / .NET / runtime installer — a single self-contained `.exe`
- Static CRT → **no VC++ redistributable** required
- UTF‑16 internally, full supplementary-plane (surrogate-pair) support
- Fast startup, small executable
- Runs in the system tray; toggle on/off with a double-click
- **JSON layout** with Shift/AltGr levels, dead keys, ligatures, and Caps-Lock
  policy — **hot-reloads** on save, no recompile
- Bundled canonical **normalization** (combining-mark ordering + composition)
- Portable core with a **unit-test suite** (runs in CI on every push)

---

## How it works

The app installs a global low-level keyboard hook (`WH_KEYBOARD_LL`). When
enabled, a mapped key is swallowed and the corresponding Old Uyghur character is
injected via `SendInput` with `KEYEVENTF_UNICODE`. This avoids COM/IME/KLC
registration entirely, so there is nothing to "install" — just run the EXE.

```
key + modifiers ─▶ Composer (dead keys · compose · ligatures · NFC) ─▶ UTF-16 ─▶ SendInput
                      ▲
              KeyboardLayout ◀ Parser ◀ Validator ◀ Registry ◀ Manager ◀ layouts/*.json
```

### Architecture

The mapping logic lives in a **Win32-free core** (`src/core/`) so it can be
unit-tested on any platform; a thin Windows layer does the hooking and I/O.

| Module            | Responsibility                                             |
|-------------------|------------------------------------------------------------|
| `Application`     | Tray icon, layout-switch menu, message loop, hot-reload    |
| `KeyboardEngine`  | Low-level hook; modifier/Caps detection; `SendInput`       |
| `core::Composer`  | Dead keys, compose sequences, ligatures, NFC reconciliation|
| `core::KeyboardLayout` | Parsed layout: levels, dead keys, compose, ligatures  |
| `core::LayoutParser`   | JSON → layout (structural parsing)                    |
| `core::LayoutValidator`| Semantic checks: dead-key refs, dupes, compose ambiguity |
| `core::LayoutRegistry` | Discover `*.json` layouts in a directory              |
| `core::KeyboardLayoutManager` | Active layout + runtime switching + reload     |
| `core::Normalizer`     | Bundled canonical ordering + composition (NFC)        |
| `core::Unicode` / `core::vk` | UTF conversions; key-name ↔ VK resolution       |
| `Config` / `Logger` | Settings + logging under `%APPDATA%\AncientUyghurKeyboard` |

```
Application ─owns─▶ KeyboardLayoutManager ─owns─▶ LayoutRegistry ─▶ Parser + Validator
     │                        │  (active KeyboardLayout*, change callback)
     ├─ SetLayout ────────────┘────────────────▶ KeyboardEngine ─owns─▶ Composer
     └─ hot reload / tray menu ▶ switchTo(id) / reloadCurrent()
```

Switch layouts at runtime from the tray (right-click ▶ **Layout**). Add a
`.json` file to the `layouts\` folder and it appears in the menu automatically.

> **Normalization & ligature caveat.** Because a hook-based keyboard cannot read
> the target app's text buffer, cross-keystroke normalization/ligatures are
> reconciled by emitting Backspaces against a locally-tracked window. This is
> best-effort and editor-dependent. Full Unicode NFC needs the whole UCD; the
> bundled table covers Old Uyghur marks + common Latin and is extensible.

---

## Build

### Requirements
- Windows 10 or 11
- Visual Studio 2022 (MSVC toolset) or Build Tools — **or** MinGW-w64 (GCC 13+)
- CMake ≥ 3.20

The app links `user32 gdi32 shell32 advapi32 ole32 uuid`. The C++20 sources
build cleanly under both **MSVC** and **GCC 14** (`-std=c++20 -Wall -Wextra`,
warning-free); the 74-test suite passes on both. The single source of truth for
the version is the top-level **`VERSION`** file (read by CMake, `AppVersion.hpp`,
the installer, and CI).

### Commands
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure   # run unit tests
```

The executable is produced at `build\Release\AncientUyghurKeyboard.exe` with a
`layouts\` folder copied alongside it.

The portable core (`src/core/`) and its tests build on any platform, so you can
run the test suite without Windows:
```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

### Building the installer & portable ZIP

Stage the payload, then build the two packages (matches what CI does):
```powershell
# 1. Stage payload
mkdir dist
copy build\Release\AncientUyghurKeyboard.exe dist\
xcopy /E /I layouts dist\layouts
copy LICENSE dist\LICENSE ; copy README.md dist\README.md

# 2. Installer (needs Inno Setup 6: `choco install innosetup`)
iscc "/DAppVersion=$(Get-Content VERSION)" "/DSourceDir=..\dist" installer\AncientUyghurKeyboard.iss
#    -> installer\Output\AncientUyghurKeyboard_Setup.exe

# 3. Portable ZIP (payload + portable.ini)
copy installer\portable.ini dist\portable.ini
Compress-Archive dist\* AncientUyghurKeyboard-portable.zip
```

---

## Usage

1. Run `AncientUyghurKeyboard.exe`. A tray icon appears.
2. Typing is **enabled** by default. Double-click the tray icon (or use the
   right-click menu) to toggle on/off.
3. Right-click ▶ **Exit** to quit.

Settings persist to `%APPDATA%\AncientUyghurKeyboard\config.ini`.
Logs are written to `%APPDATA%\AncientUyghurKeyboard\app.log`.

---

## Installation & deployment

Two ways to run it — both 100% offline, no Python/.NET/Java or runtime.

### Installer (recommended)

Run **`AncientUyghurKeyboard_Setup.exe`**. It installs into Program Files,
creates a Start-Menu shortcut (Desktop and run-at-sign-in shortcuts are
optional), and registers an uninstaller. It supports **per-user** (no admin) or
**per-machine** (elevated) installation, detects a previous version, and
upgrades in place.

Your configuration and keyboard layouts live in
`%APPDATA%\AncientUyghurKeyboard` and are **never** modified by install,
upgrade, or uninstall — settings and custom layouts always survive.

### Portable version

Download the **portable ZIP** and extract it anywhere (USB stick, network
share, a folder you control). Because it contains a `portable.ini` marker, the
app keeps `config.ini`, `app.log`, the version marker, and `layouts\` **next to
the executable** instead of `%APPDATA%`. Delete `portable.ini` to switch it back
to per-user mode.

### Upgrading

Run a newer installer (or replace the portable folder). On first launch after an
upgrade the app detects the version change, migrates `config.ini` to the current
schema (preserving every value, including keys it doesn't recognise), seeds any
new bundled layouts **without** overwriting your edits, recreates missing
folders, and shows a tray notification. Downgrades are detected and left alone.

### Uninstalling

Use **Settings ▶ Apps** or the Start-Menu *Uninstall* entry. To also remove your
settings, delete `%APPDATA%\AncientUyghurKeyboard` afterwards (for the portable
version, just delete the folder).

### Silent install / uninstall

The installer is standard Inno Setup, so unattended deployment works out of the
box:

```powershell
# Silent per-machine install (elevated), with a Desktop icon:
AncientUyghurKeyboard_Setup.exe /VERYSILENT /ALLUSERS /TASKS="desktopicon" /NORESTART

# Silent per-user install (no admin):
AncientUyghurKeyboard_Setup.exe /VERYSILENT /CURRENTUSER

# Silent uninstall:
"%ProgramFiles%\AncientUyghurKeyboard\unins000.exe" /VERYSILENT
```

`/SILENT` shows a progress bar; `/VERYSILENT` shows nothing. Exit code 0 = success.

---

## Adding / editing layouts

Layouts are JSON files in the `layouts\` folder next to the EXE. Editing and
saving the active layout **hot-reloads it live** (the app polls the file and
shows a tray notification on success/failure). Codepoints may be written as
`"U+XXXX"`, `"0xXXXX"`, a decimal number, or literal text.

```jsonc
{
  "meta": {
    "id": "old_uyghur",                 // stable identifier (defaults to filename stem)
    "name": "Old Uyghur (Phonetic)",    // display name (tray menu)
    "language": "oui",                  // BCP-47 / ISO 639-3 language tag
    "description": "Phonetic layout…",  // free text
    "author": "KutadguBilim",           // author credit
    "version": 4                        // integer layout version
  },
  "behavior": { "caps_mode": "ignore", "normalize": true },
  "keys": {
    "A": { "base": "U+10F70" },
    "S": { "base": "U+10F7B", "shift": "U+10F7F" },        // Shift level
    "OEM_2": { "base": "/", "altgr": { "compose": true } }, // AltGr starts a compose seq
    "OEM_4": { "base": "U+10F82", "shift": "U+10F84" },
    "OEM_3": { "base": { "dead": "dots_above" } }           // dead key
  },
  "dead_keys": {
    "dots_above": {
      "standalone": "U+10F84",
      "compose": { "U+10F70": ["U+10F70", "U+10F84"] }
    }
  },
  "ligatures": [ { "sequence": ["U+10F78","U+10F70"], "result": ["U+10F78","U+10F70"] } ],
  "compose_sequences": [ { "keys": ["U+10F70","U+10F76"], "output": ["U+10F70","U+10F84"] } ]
}
```

**Metadata** (`meta`): `id`, `name`, `language`, `description`, `author`, `version`.
**Per key** (`keys`): the four modifier levels `base`, `shift`, `altgr`,
`shift_altgr`; each level is an output (literal string, `"U+XXXX"`/`"0xXXXX"`
codepoint token, or an array of codepoints) **or** an action object
`{ "dead": "<id>" }` / `{ "compose": true }`. Letter keys are governed by
`behavior.caps_mode` (`ignore` | `shift_letters` | `invert`).

**Validation.** Layouts are parsed then semantically validated: duplicate key
mappings and dangling dead-key references are **errors** (the layout is
rejected); duplicate glyphs, unknown key names, and ambiguous compose sequences
are **warnings** (logged, layout still loads). Invalid layouts appear greyed-out
in the tray menu.

Set the startup layout in `config.ini` via `layout=<id>` (loads
`layouts/<id>.json`); it is loaded automatically at startup, and switching from
the tray persists the choice. Two layouts ship by default:
`old_uyghur.json` (phonetic) and `old_uyghur_qwerty.json` (positional).

---

## Continuous integration

`.github/workflows/build.yml` runs on `windows-latest` + MSVC and, on every
push, it: configures + builds **Release**, runs the **unit tests** (`ctest`),
stages the payload, builds the **portable ZIP**, builds the **installer** with
Inno Setup, and uploads three artifacts — the bare `.exe`, the
`AncientUyghurKeyboard_Setup.exe`, and the portable `.zip`. Pushing a version
tag (e.g. `v0.4.0`) additionally publishes a **GitHub Release** with the
installer, portable ZIP, and executable attached.

---

## Font note

To *see* the glyphs you must have an Old Uyghur–capable font installed (e.g.
**Noto Sans Old Uyghur**). The app produces correct Unicode regardless of the
font present; rendering is up to the target application.

---

## License

MIT — see `LICENSE`.
