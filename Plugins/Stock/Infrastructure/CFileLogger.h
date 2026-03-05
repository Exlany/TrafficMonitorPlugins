#pragma once

#include "../Core/ILogger.h"
#include <mutex>
#include <fstream>
#include <atomic>

namespace StockPlugin {
namespace Infrastructure {

/// @brief 文件日志实现
/// @details 线程安全的文件日志记录器，支持日志去重
class CFileLogger : public Core::ILogger
{
public:
    CFileLogger();
    ~CFileLogger() override;

    /// @brief 设置日志文件路径
    /// @param path 日志文件路径
    void SetLogPath(const std::wstring& path);

    /// @brief 获取日志文件路径
    const std::wstring& GetLogPath() const { return m_logPath; }

    // ILogger 接口实现
    void Log(Core::LogLevel level, const std::wstring& message) override;
    void Log(Core::LogLevel level, const std::string& message) override;
    void SetMinLevel(Core::LogLevel level) override;
    Core::LogLevel GetMinLevel() const override;

    /// @brief 启用/禁用日志去重
    /// @param enable true 启用去重（相同内容不重复记录）
    void SetDeduplication(bool enable) { m_deduplication = enable; }

    /// @brief 刷新日志缓冲区
    void Flush();

private:
    void WriteToFile(const std::string& formattedMessage);
    std::string FormatMessage(Core::LogLevel level, const std::string& message);
    static const char* LevelToString(Core::LogLevel level);
    static std::string& GetLastMessage();  // 获取线程本地的上一条消息

private:
    std::wstring m_logPath;
    std::mutex m_mutex;
    std::atomic<Core::LogLevel> m_minLevel{Core::LogLevel::Info};
    bool m_deduplication{true};
};

} // namespace Infrastructure
} // namespace StockPlugin
