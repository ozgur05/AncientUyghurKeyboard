# Architecture

The Ancient Uyghur Keyboard is a native Win32 / C++20 project with a strict
separation between a **portable, unit-tested core** and thin **OS-specific
layers**. Everything that can be tested without Windows is, and the Win32 code
is kept as small as possible.

## Layered overview

```
                       ┌──────────────────────────────────────────────┐
                       │  core/  (Win32-free, unit-tested)             │
                       │                                              │
   layouts/*.json ───▶ │  LayoutParser → KeyboardLayout ◀─ Serializer │
                       │        │            ▲                        │
                       │  LayoutValidator    │  LayoutRegistry ▲      │
                       │                     │  KeyboardLayoutManager │
                       │  Composer ──uses──▶ Normalizer, Unicode, vk  │
                       │  Version / InstallationDetector / Migration  │
                       └───────▲───────────────▲───────────────▲──────┘
                               │               │               │
        ┌──────────────────────┴───┐   ┌───────┴────────┐  ┌───┴───────────────┐
        │  Keyboard hook app        │   │ Layout Designer │  │  TSF text service  │
        │  (WH_KEYBOARD_LL,         │   │ (Win32 GUI exe) │  │  (COM in-proc DLL) │
        │   SendInput, tray)        │   │                 │  │                    │
        │  AncientUyghurKeyboard.exe│   │ …Designer.exe   │  │ …Tsf.dll           │
        └───────────────────────────┘   └─────────────────┘  └────────────────────┘
```

The three frontends (hook app, designer, TSF DLL) all consume the same `core`
static library and share layout files under `%APPDATA%\AncientUyghurKeyboard\
layouts`.

## The core (`src/core/`)

Portable C++20, no `<windows.h>`. Compiled into the `auk_core` static library
and linked by every frontend and by the test/benchmark executables.

| Module | Responsibility |
|---|---|
| `KeyboardLayout` | In-memory layout model: keys × modifier levels, dead keys, ligatures, compose sequences (O(1) hash-indexed), metadata, Caps-Lock/normalize flags |
| `LayoutParser` | JSON → `KeyboardLayout` (structural parsing, scalar validation) |
| `LayoutValidator` | Semantic validation: dangling dead keys, duplicate/non-scalar glyphs, ambiguous compose sequences |
| `LayoutSerializer` | `KeyboardLayout` → JSON (round-trip stable with the parser) |
| `LayoutRegistry` | Discover `*.json` in a directory; load/validate by id |
| `KeyboardLayoutManager` | Active layout + runtime switching with a change callback |
| `Composer` | The input state machine: dead keys, compose mode, ligatures, NFC reconciliation → `EmitOp` (backspaces + text) |
| `Normalizer` | Curated canonical ordering + composition (scoped NFC) |
| `Unicode` | UTF-8/16/32 conversions, scalar validation |
| `VirtualKeys` | VK name ↔ code |
| `UnicodeNames` / `KeyCaps` | Designer support: name search, ANSI/ISO geometry |
| `Version` / `InstallationDetector` / `Migration` | Install/upgrade classification and config/layout migration |

### The Composer — the shared brain

Every frontend translates a physical key into a `core::KeyInput`
(`{vk, shift, altgr, caps}`) and calls `Composer::process`, which returns an
`EmitOp {suppress, backspaces, insert}`. Dead keys, compose sequences, ligature
collapsing, and NFC reconciliation all live here, so all three backends behave
identically and the behavior is covered by unit tests.

> **Reconciliation model.** Because a hook/TSF backend cannot read the target
> document, cross-keystroke normalization/ligatures are reconciled by emitting
> Backspaces against a locally-tracked window. This is best-effort and
> editor-dependent; it is documented in `Composer.hpp`.

## Frontends

- **Keyboard hook app** (`src/`, `AncientUyghurKeyboard.exe`): installs a global
  `WH_KEYBOARD_LL` hook, feeds keys to the Composer, and injects the `EmitOp` via
  `SendInput`/`KEYEVENTF_UNICODE`. Owns the tray UI, config, logging, hot-reload,
  and the installer/first-run integration. This is the default backend.
- **Layout Designer** (`src/designer/`, `AncientUyghurDesigner.exe`): a Win32 GUI
  over `LayoutDocument` (edit + undo/redo + validation) with an owner-drawn
  canvas, Unicode picker, and a live preview that runs the Composer.
- **TSF text service** (`src/tsf/`, `AncientUyghurTsf.dll`): a COM in-proc server
  implementing the standard TSF interface set; applies `EmitOp`s through an edit
  session. Optional, opt-in via `regsvr32`; the hook remains the fallback.

## Data & configuration

- **Layouts**: JSON files. User layouts live under `%APPDATA%\…\layouts`; the
  installer/portable payload seeds the bundled ones there on first run.
- **Config**: `config.ini` (key=value) and `app.log` under the resolved data
  root (`%APPDATA%\…` or the exe folder in portable mode).
- **Version marker**: `installed_version.txt` drives install/upgrade detection.

## Build & release

CMake builds `auk_core`, the three frontends, tests, and the benchmark. The
single source of truth for the version is the top-level `VERSION` file (consumed
by CMake, `AppVersion.hpp`/resources, the installer, and CI). GitHub Actions
builds Debug+Release, runs the tests, runs static analysis, and packages the
installer / portable ZIP / source archive / checksums / release notes.

See [`API_REFERENCE.md`](API_REFERENCE.md) for class-level documentation and
[`DEVELOPER_GUIDE.md`](DEVELOPER_GUIDE.md) for building and debugging.
