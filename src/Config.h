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
    LogLevel     GetLogLevel() const { return m_logLevel; }

    // Absolute paths derived from the executable / %APPDATA% locations.
    static std::wstring AppDataDir();      // %APPDATA%\AncientUyghurKeyboard
    static std::wstring ExeDir();          // folder containing the .exe
    std::wstring        ConfigPath() const;
    std::wstring        LogPath()    const;
    std::wstring        LayoutPath() const; // ExeDir()\layouts\<name>.json

private:
    bool         m_enabled    = true;
    std::wstring m_layoutName = L"old_uyghur";
    LogLevel     m_logLevel   = LogLevel::Info;
};
