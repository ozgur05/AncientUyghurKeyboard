# Roadmap

This roadmap is indicative, not a commitment. Priorities are driven by community
feedback and real-world testing on Windows.

## Shipped in 1.0.0

- Old Uyghur mapping core (dead keys, compose, ligatures, scoped NFC).
- Keyboard-hook backend, tray app, hot-reload, live layout switching.
- JSON layout format + two bundled layouts.
- Installer, portable build, first-run migration, single-instance.
- Performance/hardening pass, diagnostics, benchmark.
- Release automation (CI, packaging, checksums, optional signing).
- Graphical Layout Designer.
- Native TSF IME (optional second backend).

## Near-term (1.1.x) — validation & polish

- **Runtime validation on Windows** of the two frontends compiled but not yet
  exercised here: the Layout Designer GUI interactions and the TSF service after
  `regsvr32` + language-bar activation. This is the top priority (see Known
  Limitations in the release notes).
- **App icon / branding** for the tray app, designer, and installer (currently
  the default Windows icon).
- **TSF composition display**: show pending dead-key/compose state as an
  in-progress composition (underlined) before commit, rather than committing
  immediately.
- **Run-at-startup toggle** from the app's tray menu (complementing the
  installer's optional startup shortcut).

## Mid-term (1.2.x) — layouts & correctness

- **Expanded normalization tables** and optional fuller NFC coverage.
- **Layout linting** improvements in the designer (contextual warnings, quick
  fixes) and a diff/preview when hot-reloading.
- **Additional bundled layouts** and a community layout gallery format.
- **Localized UI** for the designer.

## Longer-term (2.x) — scope

- **Vertical rendering awareness** helpers for Old Uyghur's native top-to-bottom
  orientation (documentation + tooling; actual shaping remains the host app's
  responsibility).
- **Additional historical scripts** reusing the same engine (the core is
  script-agnostic; layouts and name tables are data).
- **Signed release builds by default** once a code-signing certificate is
  available.

## Non-goals

- Network features, telemetry, or auto-update — the project is and stays 100%
  offline.
- External runtime dependencies (Python/.NET/Java/third-party libraries).
- Full Unicode Character Database embedding (the curated tables stay scoped and
  extensible).

Have a proposal? Open a **Feature request** issue.
