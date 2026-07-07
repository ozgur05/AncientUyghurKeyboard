#include "Config.h"

#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace {
std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

std::string Trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
} // namespace

std::wstring Config::AppDataDir()
{
    wchar_t* raw = nullptr;
    std::wstring dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &raw))) {
        dir = raw;
        CoTaskMemFree(raw);
    }
    dir += L"\\AncientUyghurKeyboard";
    CreateDirectoryW(dir.c_str(), nullptr); // no-op if it already exists
    return dir;
}

std::wstring Config::ExeDir()
{
    wchar_t buf[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring path(buf);
    size_t slash = path.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? L"." : path.substr(0, slash);
}

std::wstring Config::DataRoot() const { return m_dataRoot.empty() ? AppDataDir() : m_dataRoot; }
std::wstring Config::ConfigPath() const { return DataRoot() + L"\\config.ini"; }
std::wstring Config::LogPath()    const { return DataRoot() + L"\\app.log"; }
std::wstring Config::LayoutsDir() const { return DataRoot() + L"\\layouts"; }
std::wstring Config::LayoutPath() const { return LayoutsDir() + L"\\" + m_layoutName + L".json"; }

void Config::Load()
{
    // std::filesystem::path accepts a wide (UTF-16) path on both MSVC and
    // libstdc++/MinGW; a bare std::wstring only works on MSVC's extension.
    std::ifstream in(std::filesystem::path(ConfigPath()), std::ios::binary);
    if (!in.is_open()) {
        Save(); // write defaults
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        std::string s = Trim(line);
        if (s.empty() || s[0] == '#' || s[0] == '[') continue;
        auto eq = s.find('=');
        if (eq == std::string::npos) continue;
        std::string key = Trim(s.substr(0, eq));
        std::string val = Trim(s.substr(eq + 1));

        if (key == "enabled") {
            m_enabled = (val == "1" || val == "true");
        } else if (key == "layout") {
            m_layoutName = Utf8ToWide(val);
        } else if (key == "log_level") {
            if      (val == "trace") m_logLevel = LogLevel::Trace;
            else if (val == "info")  m_logLevel = LogLevel::Info;
            else if (val == "warn")  m_logLevel = LogLevel::Warn;
            else if (val == "error") m_logLevel = LogLevel::Error;
            else if (val == "off")   m_logLevel = LogLevel::Off;
        }
    }
}

void Config::Save() const
{
    std::ofstream out(std::filesystem::path(ConfigPath()), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return;

    const char* lvl = "info";
    switch (m_logLevel) {
        case LogLevel::Trace: lvl = "trace"; break;
        case LogLevel::Info:  lvl = "info";  break;
        case LogLevel::Warn:  lvl = "warn";  break;
        case LogLevel::Error: lvl = "error"; break;
        case LogLevel::Off:   lvl = "off";   break;
    }

    out << "# AncientUyghurKeyboard configuration\n";
    out << "enabled="   << (m_enabled ? "1" : "0") << "\n";
    out << "layout="    << WideToUtf8(m_layoutName) << "\n";
    out << "log_level=" << lvl << "\n";
}
