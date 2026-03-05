#pragma once

#include "../Core/IHttpClient.h"
#include "../Utils/StringUtils.h"
#include <functional>

namespace StockPlugin {
namespace Infrastructure {

/// @brief Windows HTTP 客户端实现
/// @details 基于 MFC CInternetSession 实现 HTTP 请求
class CWinHttpClient : public Core::IHttpClient
{
public:
    CWinHttpClient();
    ~CWinHttpClient() override;

    // IHttpClient 接口实现
    bool Get(const std::wstring& url, std::string& response,
             const Core::HttpRequestOptions& options = {}) override;
    bool Download(const std::wstring& url, const std::wstring& savePath,
                  const Core::HttpRequestOptions& options = {}) override;
    bool IsNetworkAvailable() override;

    // 扩展方法
    /// @brief 设置系统代码页（用于编码转换）
    void SetSystemCodePage(UINT codePage);

    /// @brief 设置日志回调
    void SetLogCallback(std::function<void(const std::wstring&)> callback);

    // 静态工具方法（委托给 StringUtils，保持向后兼容）
    /// @brief 将 const char* 转换为宽字符串
    /// @deprecated 请使用 Utils::StringUtils::ToUnicode
    static std::wstring StrToUnicode(const char* str, bool utf8 = false)
    {
        return Utils::StringUtils::ToUnicode(str, utf8);
    }

    /// @brief 将宽字符串转换为 char*
    /// @deprecated 请使用 Utils::StringUtils::FromUnicode
    static std::string UnicodeToStr(const wchar_t* wstr, bool utf8 = false)
    {
        return Utils::StringUtils::FromUnicode(wstr, utf8);
    }

    /// @brief URL 编码
    /// @deprecated 请使用 Utils::StringUtils::URLEncode
    static std::wstring URLEncode(const std::wstring& wstr)
    {
        return Utils::StringUtils::URLEncode(wstr);
    }

private:
    UINT m_systemCodePage{936};  // 默认 GBK
    std::function<void(const std::wstring&)> m_logCallback;

    void Log(const std::wstring& message);
};

} // namespace Infrastructure
} // namespace StockPlugin
