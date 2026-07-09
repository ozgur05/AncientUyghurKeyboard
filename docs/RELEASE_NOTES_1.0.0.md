# Release Notes — v1.0.0

**Ancient Uyghur Keyboard 1.0.0** — a 100% offline, native Win32 / C++20 keyboard
and input method for the **Old Uyghur** script (Unicode `U+10F70–U+10F89`).
No Python, .NET, Java, or external runtime; a single self-contained executable
(static CRT, no VC++ redistributable).

## Highlights

- **Type Old Uyghur anywhere** via a global keyboard hook — just run the EXE,
  nothing to install.
- **Rich layout format**: Base/Shift/AltGr/Shift+AltGr levels, dead keys, compose
  sequences, ligatures, Caps-Lock policy, and metadata, in editable JSON.
- **Graphical Layout Designer** to create/edit layouts visually, with a Unicode
  picker, live preview, and validation.
- **Native TSF IME** (`AncientUyghurTsf.dll`) as an optional second backend that
  integrates with the Windows language bar.
- **Professional installer** (per-user/per-machine, silent, upgrade-safe) and a
  **portable ZIP**; settings and custom layouts always survive upgrades.
- **Fast & hardened**: O(1) compose matching, validated Unicode input, RAII
  resource management, diagnostics, and a benchmarked hot path.

## Downloads

Each release attaches: `AncientUyghurKeyboard_Setup.exe` (installer),
`AncientUyghurKeyboard_Portable.zip` (portable), `AncientUyghurKeyboard_Source.zip`,
and `SHA256SUMS.txt`. Verify checksums before running.

## Getting started

- **Installer**: run `AncientUyghurKeyboard_Setup.exe`, then use the tray icon.
- **Portable**: unzip and run `AncientUyghurKeyboard.exe` (keeps data in-folder).
- **Designer**: tray menu → *Layout Designer…*, or run `AncientUyghurDesigner.exe`.
- **TSF IME**: `regsvr32 AncientUyghurTsf.dll` (elevated), then pick
  *Ancient Uyghur (TSF)* from the language switcher.

See the [README](../README.md) for full usage and layout authoring.

## Migration notes

- **First install of 1.0.0**: nothing to migrate; bundled layouts are seeded into
  `%APPDATA%\AncientUyghurKeyboard\layouts` on first run.
- **Upgrading from a pre-release build**: on first launch the app detects the
  version change and migrates `config.ini` to the current schema **preserving all
  your settings** (including keys it doesn't recognize), seeds any new bundled
  layouts **without overwriting your edits**, and recreates missing folders. Your
  `%APPDATA%` data is never touched by the installer/uninstaller.
- **Config schema**: the legacy `enable` key is auto-renamed to `enabled`. No
  manual action required.
- **Layout format**: unchanged and stable; layouts written by the designer
  re-parse identically. No layout migration needed.

## Known limitations

- **Runtime verification of the GUI designer and the TSF DLL is pending.** Both
  are compiled and link-verified and share the unit-tested core engine, but the
  designer's window interactions and the TSF service (after registration +
  language-bar activation) have not yet been exercised end-to-end on a target
  desktop. Validating these is the top item on the 1.1 roadmap.
- **Normalization/ligature reconciliation is best-effort and editor-dependent.**
  Because a hook/IME cannot read the target document, cross-keystroke edits are
  reconciled with Backspaces against a locally-tracked window; behavior can vary
  by application (especially for supplementary-plane characters).
- **Scoped NFC.** Normalization uses a curated table (Old Uyghur marks + common
  Latin), not the full Unicode Character Database.
- **Glyph rendering is the host app's job.** Install an Old Uyghur–capable font
  (e.g. *Noto Sans Old Uyghur*) to see the characters; the app always emits
  correct Unicode regardless of the font present.
- **TSF composition** commits inserted text directly (no persistent underlined
  composition yet — planned for 1.1).

## Future roadmap

See [`ROADMAP.md`](ROADMAP.md). Near-term: runtime validation of the designer +
TSF paths, app icon/branding, TSF composition display, and a run-at-startup
toggle.

## Thanks

Built in the open. Contributions welcome — see
[`../CONTRIBUTING.md`](../CONTRIBUTING.md).
