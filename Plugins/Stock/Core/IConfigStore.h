#pragma once

#include <string>

namespace StockPlugin {
namespace Core {

/// @brief 配置存储接口
/// @details 抽象配置的读写操作，支持依赖注入和单元测试
class IConfigStore
{
public:
    virtual ~IConfigStore() = default;

    /// @brief 加载配置文件
    /// @param path 配置文件路径
    virtual void Load(const std::wstring& path) = 0;

    /// @brief 保存配置到文件
    virtual void Save() = 0;

    /// @brief 获取整数配置
    /// @param section 配置节名
    /// @param key 配置键名
    /// @param defaultValue 默认值
    /// @return 配置值
    virtual int GetInt(const wchar_t* section, const wchar_t* key, int defaultValue) = 0;

    /// @brief 获取布尔配置
    virtual bool GetBool(const wchar_t* section, const wchar_t* key, bool defaultValue) = 0;

    /// @brief 获取字符串配置
    virtual std::wstring GetString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue) = 0;

    /// @brief 设置整数配置
    virtual void SetInt(const wchar_t* section, const wchar_t* key, int value) = 0;

    /// @brief 设置布尔配置
    virtual void SetBool(const wchar_t* section, const wchar_t* key, bool value) = 0;

    /// @brief 设置字符串配置
    virtual void SetString(const wchar_t* section, const wchar_t* key, const wchar_t* value) = 0;

    /// @brief 获取配置文件路径
    virtual const std::wstring& GetPath() const = 0;
};

} // namespace Core
} // namespace StockPlugin
