# Pushing & CI validation

This project has been developed and verified **locally with GCC/MinGW only**.
The GitHub Actions workflow (`.github/workflows/build.yml`) is the first build
under **MSVC** and the first run of the **Inno Setup** installer. This document
is the checklist for getting it onto GitHub and reading the first CI result.

## 1. Create the remote and push

Using the GitHub CLI (if installed and authenticated):

```bash
gh repo create AncientUyghurKeyboard --public --source . --remote origin --push
```

Or manually, after creating an empty repo on github.com:

```bash
git remote add origin https://github.com/<you>/AncientUyghurKeyboard.git
git push -u origin main
```

Pushing to `main` triggers the **Build, Test, Package** workflow automatically.

## 2. What the workflow does

The pipeline has three jobs (runner: `windows-latest`, MSVC toolset):

**`build`** — matrix over **Debug** and **Release**: configure with CMake +
Visual Studio 2022, build, and run the unit tests with `ctest`. Any failing
test fails the job.

**`quality`** — installs cppcheck and runs static analysis (advisory: reports
findings without blocking). `.clang-format` and `.clang-tidy` are provided for
local use.

**`package`** (needs `build`) — produces the full release set:

1. Read the version from `VERSION`; build + test **Release**.
2. Stage `dist/` (exe + `layouts/` + `LICENSE` + `README.md` + PDB).
3. Build the **portable ZIP** (payload + `portable.ini`).
4. Create the **source archive** (`git archive`).
5. Install **Inno Setup** and build `AncientUyghurKeyboard_Setup.exe`.
6. **Code sign** the installer + exe *if* the `SIGN_PFX_BASE64` secret is set
   (SHA-256 + RFC-3161 timestamp, verified); otherwise ship unsigned.
7. Generate `ReleaseNotes.md` from the git log and `SHA256SUMS.txt`.
8. Run `scripts/verify-release.ps1` to confirm artifacts + checksums.
9. Upload all artifacts (installer, portable ZIP, source ZIP, checksums,
   release notes, PDB).

On a version tag (`v0.4.0`, …) it additionally publishes a **GitHub Release**
with the installer, portable/source ZIPs, and checksums attached. Tag with:

```bash
git tag v0.4.0
git push origin v0.4.0
```

## 3. First-run watch-list

These are the steps most likely to need attention on the very first CI run,
because they cannot be exercised with the local GCC toolchain:

- **MSVC compile** — the code builds warning-free under GCC `-Wall -Wextra`;
  MSVC `/W4` may surface additional warnings (not errors). They will not fail
  the build unless `/WX` is added (it is not).
- **`rc.exe` (resource compiler)** — `resources.rc` includes `AppVersion.hpp`
  and uses only plain macros and a literal version string (no `#`/`##`
  preprocessor operators), which `rc.exe` supports.
- **Inno Setup** — `ISCC.exe` is located under `Program Files (x86)\Inno Setup 6`
  after the Chocolatey install; the step throws a clear error if it is missing.
- **Release permissions** — the workflow declares `permissions: contents: write`
  so the tag-triggered release step can upload assets.

## 4. Code signing (optional)

Signing is **opt-in** and off by default, so the pipeline works with no secrets.
To sign released binaries, add two repository secrets:

- `SIGN_PFX_BASE64` — your Authenticode code-signing certificate (`.pfx`),
  base64-encoded (`[Convert]::ToBase64String([IO.File]::ReadAllBytes('cert.pfx'))`).
- `SIGN_PFX_PASSWORD` — the certificate password.

When present, the package job signs `AncientUyghurKeyboard_Setup.exe` and the
application exe with `signtool` (SHA-256, RFC-3161 timestamp) and verifies each
signature. When absent, artifacts are shipped unsigned and the step is a no-op.

## 5. Local developer build

See the **Build** section of `README.md` for building the app, running the
tests, and producing the installer and portable ZIP locally. Verify a local
release payload with:

```powershell
pwsh scripts/verify-release.ps1 -Dir . -Version (Get-Content VERSION)
```
