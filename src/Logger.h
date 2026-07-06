// Logger.h — minimal, thread-safe, dependency-free file logger.
#pragma once

#include <string>
#include <mutex>
#include <fstream>

enum class LogLevel {
    Trace = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
    Off   = 4
};

class Logger {
public:
    // Process-wide singleton.
    static Logger& Instance();

    // (Re)open the log file and set the minimum level to record.
    void Init(const std::wstring& path, LogLevel level);

    void Log(LogLevel level, const std::wstring& msg);

    void Trace(const std::wstring& m) { Log(LogLevel::Trace, m); }
    void Info (const std::wstring& m) { Log(LogLevel::Info,  m); }
    void Warn (const std::wstring& m) { Log(LogLevel::Warn,  m); }
    void Error(const std::wstring& m) { Log(LogLevel::Error, m); }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;

    static const wchar_t* LevelName(LogLevel level);

    std::mutex    m_mutex;
    std::ofstream m_file;                 // bytes; we write UTF-8
    LogLevel      m_level = LogLevel::Info;
};
