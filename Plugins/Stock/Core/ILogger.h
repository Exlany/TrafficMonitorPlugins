#pragma once

#include <string>

namespace StockPlugin {
namespace Core {

/// @brief 日志级别
enum class LogLevel
{
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

/// @brief 日志接口
/// @details 定义日志记录的抽象接口，支持不同的日志实现
class ILogger
{
public:
    virtual ~ILogger() = default;

    /// @brief 记录日志
    /// @param level 日志级别
    /// @param message 日志消息
    virtual void Log(LogLevel level, const std::wstring& message) = 0;
    virtual void Log(LogLevel level, const std::string& message) = 0;

    /// @brief 便捷方法
    virtual void Debug(const std::wstring& message) { Log(LogLevel::Debug, message); }
    virtual void Info(const std::wstring& message) { Log(LogLevel::Info, message); }
    virtual void Warning(const std::wstring& message) { Log(LogLevel::Warning, message); }
    virtual void Error(const std::wstring& message) { Log(LogLevel::Error, message); }

    virtual void Debug(const std::string& message) { Log(LogLevel::Debug, message); }
    virtual void Info(const std::string& message) { Log(LogLevel::Info, message); }
    virtual void Warning(const std::string& message) { Log(LogLevel::Warning, message); }
    virtual void Error(const std::string& message) { Log(LogLevel::Error, message); }

    /// @brief 设置最小日志级别
    virtual void SetMinLevel(LogLevel level) = 0;

    /// @brief 获取当前最小日志级别
    virtual LogLevel GetMinLevel() const = 0;
};

} // namespace Core
} // namespace StockPlugin
