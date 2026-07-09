# Security Policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 1.0.x   | ✅ |
| < 1.0   | ❌ (pre-release) |

## Reporting a vulnerability

**Do not open a public issue for security vulnerabilities.**

Please report privately via GitHub's **[Report a vulnerability]** feature on the
repository's *Security* tab (Security → Advisories → Report a vulnerability). If
that is unavailable, contact a maintainer directly through the channels in
[`SUPPORT.md`](SUPPORT.md).

Please include:

- affected component (keyboard hook app, TSF DLL, layout designer, installer);
- version / commit hash (shown in the app log as `Build: …`);
- a description and, if possible, a minimal reproduction.

We aim to acknowledge reports within a few days and to ship a fix or mitigation
as promptly as the severity warrants, coordinating disclosure with the reporter.

## Security properties & threat model

- **100% offline.** The application makes no network connections and bundles no
  telemetry. It has no auto-update mechanism.
- **Input validation.** All layout JSON is parsed and validated; codepoints must
  be valid Unicode scalar values (surrogates and out-of-range values are
  rejected by both the parser and the validator), so a malformed or hostile
  layout file cannot inject broken UTF-16 or reach outside the layouts folder
  (layout ids are constrained to plain file stems).
- **Least privilege.** The keyboard app runs per-user and requires no elevation.
  The installer supports per-user installs without admin rights. The TSF DLL
  registration (`regsvr32`) is the only step that may require elevation.
- **Trust boundaries.** The main untrusted inputs are (a) layout `.json` files
  and (b) `config.ini`. Both are treated as untrusted and validated. Keystrokes
  are processed in-process; no keystroke data is logged or persisted.

## Hardening notes for deployers

- Prefer installing signed builds (see the release assets / `SECURITY` of the
  release). Verify `SHA256SUMS.txt` against downloaded artifacts.
- The optional TSF IME loads a COM DLL into other processes once registered;
  register it only from a trusted, verified build.
