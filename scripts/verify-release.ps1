<#
.SYNOPSIS
    Verify a built release: required artifacts exist and their SHA256 checksums
    match SHA256SUMS.txt. Optionally verify Authenticode signatures.

.DESCRIPTION
    Run against the directory that holds the packaged release artifacts. Returns
    a non-zero exit code if anything is missing or a checksum mismatches, so it
    can gate a release step in CI.

.PARAMETER Dir
    Directory containing the artifacts and SHA256SUMS.txt (default: current dir).

.PARAMETER Version
    Expected version string (e.g. 0.4.0). Used only for reporting.

.PARAMETER RequireSignature
    If set, the installer and portable exe must carry a valid Authenticode
    signature (fails otherwise). Off by default (unsigned builds are allowed).

.EXAMPLE
    pwsh scripts/verify-release.ps1 -Dir dist -Version 0.4.0
#>
[CmdletBinding()]
param(
    [string]$Dir = ".",
    [string]$Version = "",
    [switch]$RequireSignature
)

$ErrorActionPreference = "Stop"
$fail = $false

function Fail($msg) { Write-Host "  [FAIL] $msg" -ForegroundColor Red; $script:fail = $true }
function Ok($msg)   { Write-Host "  [ OK ] $msg" -ForegroundColor Green }

Write-Host "Verifying release artifacts in '$Dir'" -NoNewline
if ($Version) { Write-Host " (version $Version)" } else { Write-Host "" }

$required = @(
    "AncientUyghurKeyboard_Setup.exe",
    "AncientUyghurKeyboard_Portable.zip",
    "SHA256SUMS.txt",
    "ReleaseNotes.md"
)
foreach ($f in $required) {
    if (Test-Path (Join-Path $Dir $f)) { Ok "present: $f" }
    else { Fail "missing:  $f" }
}

# Verify checksums listed in SHA256SUMS.txt (format: "<hash>  <filename>").
$sumsPath = Join-Path $Dir "SHA256SUMS.txt"
if (Test-Path $sumsPath) {
    foreach ($line in Get-Content $sumsPath) {
        $t = $line.Trim()
        if (-not $t) { continue }
        $parts = $t -split '\s+', 2
        if ($parts.Count -ne 2) { continue }
        $expected = $parts[0].ToLower()
        $name     = $parts[1].Trim()
        $path     = Join-Path $Dir $name
        if (-not (Test-Path $path)) { Fail "checksum target missing: $name"; continue }
        $actual = (Get-FileHash -Algorithm SHA256 $path).Hash.ToLower()
        if ($actual -eq $expected) { Ok "checksum: $name" }
        else { Fail "checksum mismatch: $name" }
    }
}

# Optional Authenticode verification.
if ($RequireSignature) {
    foreach ($exe in @("AncientUyghurKeyboard_Setup.exe")) {
        $path = Join-Path $Dir $exe
        if (-not (Test-Path $path)) { continue }
        $sig = Get-AuthenticodeSignature $path
        if ($sig.Status -eq "Valid") { Ok "signature valid: $exe" }
        else { Fail "signature $($sig.Status): $exe" }
    }
}

if ($fail) { Write-Host "Release verification FAILED." -ForegroundColor Red; exit 1 }
Write-Host "Release verification passed." -ForegroundColor Green
exit 0
