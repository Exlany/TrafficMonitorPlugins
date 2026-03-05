#pragma once

#include <string>
#include <functional>

namespace StockPlugin {
namespace Core {

/// @brief HTTP 响应结构
struct HttpResponse
{
    bool success{false};
    int statusCode{0};
    std::string body;
    std::wstring errorMessage;
};

/// @brief HTTP 请求选项
struct HttpRequestOptions
{
    std::wstring userAgent;
    std::wstring headers;
    int connectTimeout{10000};  // 连接超时(毫秒)
    int receiveTimeout{10000};  // 接收超时(毫秒)
    bool utf8{false};           // 响应是否为 UTF-8 编码
};

/// @brief HTTP 客户端接口
/// @details 抽象 HTTP 请求操作，支持依赖注入和单元测试
class IHttpClient
{
public:
    virtual ~IHttpClient() = default;

    /// @brief 发送 GET 请求
    /// @param url 请求 URL
    /// @param response 响应内容
    /// @param options 请求选项
    /// @return 是否成功
    virtual bool Get(const std::wstring& url, std::string& response,
                     const HttpRequestOptions& options = {}) = 0;

    /// @brief 下载文件
    /// @param url 下载 URL
    /// @param savePath 保存路径
    /// @param options 请求选项
    /// @return 是否成功
    virtual bool Download(const std::wstring& url, const std::wstring& savePath,
                          const HttpRequestOptions& options = {}) = 0;

    /// @brief 检查网络是否可用
    /// @return 是否可用
    virtual bool IsNetworkAvailable() = 0;
};

} // namespace Core
} // namespace StockPlugin
