# Release Notes — v1.1.3

**Ancient Uyghur Keyboard 1.1.3** — Fixes AltGr key detection, SendInput
modifier interference, and stale `%APPDATA%` layout upgrades.

## What's fixed

### 1. AltGr key detection on US English keyboards

**Root cause:** The AltGr detection logic (`const bool altgr = rAlt && ctrl`)
required both Right Alt *and* Ctrl to be held simultaneously. On European
keyboards Windows automatically synthesizes `Ctrl` when Right Alt is pressed,
but on US English layouts it does not — so Right Alt alone did nothing.

**Fix:** Changed to `altgr = rAlt || (ctrl && lAlt)` in both
`KeyboardEngine::HandleKeyDown` (hook backend) and
`CTextService::HandleKeyDown` (TSF IME). Now:
- **Right Alt alone** activates AltGr (works on all keyboard layouts)
- **Ctrl + Left Alt** also activates AltGr (alternative combo)
- Genuine `Ctrl+_` and `LeftAlt+_` shortcuts still pass through

### 2. Modifier interference during SendInput

**Root cause:** When `AltGr+\` was pressed to type Tatweel, `SendEdits()`
injected `KEYEVENTF_UNICODE` events while `VK_RMENU` and `VK_LCONTROL` were
still physically held down. Applications like LibreOffice Writer interpreted the
injected characters as Alt/Ctrl shortcut combos and silently dropped them.

**Fix:** `SendEdits()` now:
1. Detects which modifier keys are held (`GetAsyncKeyState`)
2. Queues synthetic key-up events for held modifiers before the payload
3. Queues the backspace and Unicode character events
4. Re-queues key-down events for the released modifiers

This means holding Right Alt and pressing `\` repeatedly keeps typing Tatweel
without needing to release and re-press Right Alt.

### 3. Stale `%APPDATA%` layouts after upgrade

**Root cause:** `AutoMigration::SeedLayouts()` used `skip_existing`, so
upgrading from v1.0.0 to v1.1.x left the old v1.0.0 layout files (without any
AltGr mappings) in `%APPDATA%\AncientUyghurKeyboard\layouts\`.

**Fix:** After seeding missing layouts, the migrator now:
1. Parses `meta.version` from both the bundled and user layout JSON files
2. If `bundled.meta.version > user.meta.version`:
   - Backs up the user file to `<id>.json.bak`
   - Overwrites with the newer bundled version
3. If versions are equal or the user's is higher, the file is left untouched

This uses the new `core::Migration::planLayoutUpgrade()` pure function (with
3 new unit tests).

## Files changed

| File | Change |
|------|--------|
| `src/KeyboardEngine.cpp` | AltGr detection + SendInput modifier release |
| `src/tsf/TextService.cpp` | AltGr detection |
| `src/AutoMigration.cpp` | Version-aware layout upgrade |
| `src/AutoMigration.h` | Updated layout seeding/upgrade docs |
| `src/core/Migration.hpp` | New `planLayoutUpgrade()` declaration |
| `src/core/Migration.cpp` | New `planLayoutUpgrade()` implementation |
| `src/AppVersion.hpp` | Bumped AUK version macros to 1.1.3 |
| `tests/test_migration.cpp` | 3 new upgrade-planning tests |
| `VERSION` | 1.1.1 → 1.1.3 |
| `CHANGELOG.md` | 1.1.3 entry |

## Upgrading

Run the new installer or replace the portable folder. On first launch the app
will automatically detect that the bundled layouts (v6/v3) are newer than the
user's installed versions and upgrade them (with `.bak` backup).

## Full changelog

See [`../CHANGELOG.md`](../CHANGELOG.md).
