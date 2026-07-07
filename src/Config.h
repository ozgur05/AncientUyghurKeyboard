// Config.h — persisted user settings (simple key=value file, no dependencies).
#pragma once

#include <string>
#include "Logger.h"

class Config {
public:
    // Load from disk (creating defaults if the file is missing).
    void Load();
    // Persist current values to disk.
    void Save() const;

    bool         Enabled()   const { return m_enabled; }
    void         SetEnabled(bool e) { m_enabled = e; }

    const std::wstring& LayoutName() const { return m_layoutName; }
    void         SetLayoutName(const std::wstring& n) { m_layoutName = n; }
    LogLevel     GetLogLevel() const { return m_logLevel; }

    // The writable data root (config.ini, app.log, layouts\). Resolved at
    // startup by InstallerHelper (portable dir or %APPDATA%). Defaults to
    // %APPDATA%\AncientUyghurKeyboard so Config is usable without the installer.
    void         SetDataRoot(const std::wstring& dir) { m_dataRoot = dir; }
    std::wstring DataRoot() const;

    // Absolute paths derived from the executable / %APPDATA% locations.
    static std::wstring AppDataDir();      // %APPDATA%\AncientUyghurKeyboard
    static std::wstring ExeDir();          // folder containing the .exe
    std::wstring        ConfigPath() const;
    std::wstring        LogPath()    const;
    std::wstring        LayoutsDir() const; // DataRoot()\layouts
    std::wstring        LayoutPath() const; // DataRoot()\layouts\<name>.json

private:
    bool         m_enabled    = true;
    std::wstring m_layoutName = L"old_uyghur";
    LogLevel     m_logLevel   = LogLevel::Info;
    std::wstring m_dataRoot;   // empty => AppDataDir()
};
