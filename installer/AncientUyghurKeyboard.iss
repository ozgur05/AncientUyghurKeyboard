; AncientUyghurKeyboard.iss — Inno Setup installer script.
;
; Builds AncientUyghurKeyboard_Setup.exe. Supports per-user and per-machine
; installs, silent install/uninstall, previous-version detection, and clean
; upgrades. User configuration and layouts live in %APPDATA% and are never
; touched by (un)install, so settings survive upgrades and removal.
;
; Build:  iscc /DAppVersion=1.0.0 /DSourceDir=..\dist installer\AncientUyghurKeyboard.iss
;   AppVersion : version string (defaults below; CI passes the VERSION file).
;   SourceDir  : folder containing AncientUyghurKeyboard.exe, layouts\,
;                LICENSE, README.md (defaults to ..\dist).

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\dist"
#endif

#define AppName "Ancient Uyghur Keyboard"
#define AppExeName "AncientUyghurKeyboard.exe"
#define AppPublisher "Ancient Uyghur Keyboard Project"
#define AppURL "https://github.com/KutadguBilim/AncientUyghurKeyboard"

[Setup]
; A stable AppId ties versions together so upgrades are detected. Do not change.
AppId={{7E4C7B1A-2F3D-4C7E-9A11-0A1B2C3D4E5F}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
VersionInfoVersion={#AppVersion}

; Install location + upgrade behavior. UsePreviousAppDir reuses the prior
; install directory on upgrade; the AppId above drives version detection.
DefaultDirName={autopf}\AncientUyghurKeyboard
DefaultGroupName={#AppName}
UsePreviousAppDir=yes
DisableProgramGroupPage=yes
AllowNoIcons=yes

; Per-user or per-machine: default to lowest privileges; the user (or the
; command line) can elevate to an all-users install.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline

; Close a running instance during upgrade (matches the app's CreateMutex name).
AppMutex=AncientUyghurKeyboard_SingleInstance
CloseApplications=yes
RestartApplications=no

; Output.
OutputDir=Output
OutputBaseFilename=AncientUyghurKeyboard_Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#AppExeName}
LicenseFile={#SourceDir}\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startupicon"; Description: "Start Ancient Uyghur Keyboard automatically when I sign in"; GroupDescription: "Startup:"; Flags: unchecked

[Files]
Source: "{#SourceDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\layouts\*";     DestDir: "{app}\layouts"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\LICENSE";       DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "{#SourceDir}\README.md";     DestDir: "{app}"; DestName: "README.md";   Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}";            Filename: "{app}\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}";      Filename: "{app}\{#AppExeName}"; Tasks: desktopicon
; Optional run-at-sign-in shortcut in the user's Startup folder.
Name: "{userstartup}\{#AppName}";      Filename: "{app}\{#AppExeName}"; Tasks: startupicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

; NOTE: no [UninstallDelete]. User data under %APPDATA%\AncientUyghurKeyboard
; (config.ini, app.log, installed_version.txt, user layouts) is intentionally
; left in place so settings and custom layouts survive uninstall/reinstall.
;
; Upgrade handling is Inno Setup's standard AppId-based mechanism: the stable
; AppId above lets Setup detect and replace a previous install in place. The
; application itself additionally classifies first-run/upgrade/downgrade at
; launch (see InstallationDetector) and preserves settings accordingly, so no
; custom install-time version logic is required here.
