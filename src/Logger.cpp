#include "Logger.h"
#include <windows.h>
#include <filesystem>

namespace {
std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                        s.data(), n, nullptr, nullptr);
    return s;
}
} // namespace

Logger& Logger::Instance()
{
    static Logger instance;
    return instance;
}

void Logger::Init(const std::wstring& path, LogLevel level)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_level = level;
    if (m_file.is_open())
        m_file.close();
    // std::ios::app keeps history across runs. Wrap in filesystem::path so the
    // wide (UTF-16) path works on libstdc++/MinGW as well as MSVC.
    m_file.open(std::filesystem::path(path), std::ios::app | std::ios::binary);
}

const wchar_t* Logger::LevelName(LogLevel level)
{
    switch (level) {
        case LogLevel::Trace: return L"TRACE";
        case LogLevel::Info:  return L"INFO ";
        case LogLevel::Warn:  return L"WARN ";
        case LogLevel::Error: return L"ERROR";
        default:              return L"?????";
    }
}

void Logger::Log(LogLevel level, const std::wstring& msg)
{
    if (m_level == LogLevel::Off || level < m_level)
        return;

    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_file.is_open())
        return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t ts[32];
    swprintf_s(ts, L"%04d-%02d-%02d %02d:%02d:%02d",
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    std::wstring line = std::wstring(ts) + L" [" + LevelName(level) + L"] " + msg + L"\n";
    const std::string utf8 = WideToUtf8(line);
    m_file.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    m_file.flush();
}
