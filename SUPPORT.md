# Support

Thanks for using the Ancient Uyghur Keyboard. Here is how to get help.

## Before you ask

1. Read the [README](README.md) — installation, usage, layouts, and the
   designer are covered there.
2. Check the [Developer Guide](docs/DEVELOPER_GUIDE.md) for build/debug issues.
3. Search existing [issues](https://github.com/ozgur05/AncientUyghurKeyboard/issues)
   — your question may already be answered.
4. Collect the app version: it is written to the log on every launch as
   `Build: 1.0.0+build.N (git …, built …)`. The log lives at
   `%APPDATA%\AncientUyghurKeyboard\app.log`.

## How to get help

| I want to… | Do this |
|---|---|
| Report a bug | Open an issue using the **Bug report** template |
| Request a feature | Open an issue using the **Feature request** template |
| Ask a usage question | Open a **Question** issue or start a Discussion |
| Report a security issue | Follow [`SECURITY.md`](SECURITY.md) — do **not** open a public issue |
| Contribute a fix | See [`CONTRIBUTING.md`](CONTRIBUTING.md) |

## Diagnostics that help us help you

- The relevant lines from `%APPDATA%\AncientUyghurKeyboard\app.log`.
- Which backend you use (keyboard hook, or the TSF IME).
- The layout file involved (attach the `.json`).
- Windows version (10 or 11) and whether the app is installed or portable.
- For latency issues, set the environment variable `AUK_DIAG=1` and attach the
  hook-timing lines from the log.

## Response expectations

This is a community open-source project maintained on a best-effort basis. There
is no commercial support or SLA. Well-described issues with reproduction steps
get resolved fastest.
