#pragma once

#include "../Core/IConfigStore.h"
#include <memory>

namespace utilities {
    class CIniHelper;
}

namespace StockPlugin {
namespace Infrastructure {

/// @brief INI 配置存储实现
/// @details 基于 utilities::CIniHelper 实现配置读写
class CIniConfigStore : public Core::IConfigStore
{
public:
    CIniConfigStore();
    ~CIniConfigStore() override;

    // IConfigStore 接口实现
    void Load(const std::wstring& path) override;
    void Save() override;
    int GetInt(const wchar_t* section, const wchar_t* key, int defaultValue) override;
    bool GetBool(const wchar_t* section, const wchar_t* key, bool defaultValue) override;
    std::wstring GetString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue) override;
    void SetInt(const wchar_t* section, const wchar_t* key, int value) override;
    void SetBool(const wchar_t* section, const wchar_t* key, bool value) override;
    void SetString(const wchar_t* section, const wchar_t* key, const wchar_t* value) override;
    const std::wstring& GetPath() const override;

    // 扩展方法：字符串列表
    void GetStringList(const wchar_t* section, const wchar_t* key,
                       std::vector<std::wstring>& values,
                       const std::vector<std::wstring>& defaultValue);
    void SetStringList(const wchar_t* section, const wchar_t* key,
                       const std::vector<std::wstring>& values);

private:
    std::wstring m_path;
    std::unique_ptr<utilities::CIniHelper> m_iniHelper;
};

} // namespace Infrastructure
} // namespace StockPlugin
