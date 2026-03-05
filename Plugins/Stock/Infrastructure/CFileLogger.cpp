#include "pch.h"
#include "CFileLogger.h"
#include "../Utils/StringUtils.h"
#include <Windows.h>
#include <iomanip>
#include <sstream>

namespace StockPlugin {
namespace Infrastructure {

// 使用函数内 thread_local 替代类静态成员，避免某些编译器/链接场景的问题
std::string& CFileLogger::GetLastMessage()
{
    thread_local std::string lastMessage;
    return lastMessage;
}

CFileLogger::CFileLogger() = default;

CFileLogger::~CFileLogger()
{
    Flush();
}

void CFileLogger::SetLogPath(const std::wstring& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logPath = path;
}

void CFileLogger::Log(Core::LogLevel level, const std::wstring& message)
{
    Log(level, Utils::StringUtils::FromUnicode(message, true));
}

void CFileLogger::Log(Core::LogLevel level, const std::string& message)
{
    // 检查日志级别
    if (level < m_minLevel.load())
        return;

    // 去重检查（线程独立）
    std::string& lastMessage = GetLastMessage();
    if (m_deduplication && message == lastMessage)
        return;

    lastMessage = message;

    std::string formatted = FormatMessage(level, message);
    WriteToFile(formatted);
}

void CFileLogger::SetMinLevel(Core::LogLevel level)
{
    m_minLevel.store(level);
}

Core::LogLevel CFileLogger::GetMinLevel() const
{
    return m_minLevel.load();
}

void CFileLogger::Flush()
{
    // 文件在每次写入后自动关闭，无需额外刷新
}

void CFileLogger::WriteToFile(const std::string& formattedMessage)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_logPath.empty())
        return;

    std::ofstream file(m_logPath, std::ios::app);
    if (file.is_open())
    {
        file << formattedMessage << std::endl;
    }
}

std::string CFileLogger::FormatMessage(Core::LogLevel level, const std::string& message)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    std::ostringstream oss;
    oss << st.wYear << "/"
        << std::setfill('0') << std::setw(2) << st.wMonth << "/"
        << std::setfill('0') << std::setw(2) << st.wDay << " "
        << std::setfill('0') << std::setw(2) << st.wHour << ":"
        << std::setfill('0') << std::setw(2) << st.wMinute << ":"
        << std::setfill('0') << std::setw(2) << st.wSecond << "."
        << std::setfill('0') << std::setw(3) << st.wMilliseconds
        << " [" << LevelToString(level) << "] "
        << message;

    return oss.str();
}

const char* CFileLogger::LevelToString(Core::LogLevel level)
{
    switch (level)
    {
    case Core::LogLevel::Debug:   return "DEBUG";
    case Core::LogLevel::Info:    return "INFO";
    case Core::LogLevel::Warning: return "WARN";
    case Core::LogLevel::Error:   return "ERROR";
    default:                      return "UNKNOWN";
    }
}

} // namespace Infrastructure
} // namespace StockPlugin
