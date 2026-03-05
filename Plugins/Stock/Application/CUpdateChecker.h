#pragma once

#include <string>
#include <memory>
#include <functional>
#include "../Core/IHttpClient.h"

namespace StockPlugin {
namespace Application {

/// @brief 更新信息结构
struct UpdateInfo
{
    std::wstring version;
    std::wstring downloadUrl;      // zip 下载地址
    std::wstring releasePageUrl;   // Release 页面地址
    std::wstring releaseNotes;
};

/// @brief 更新检查器
/// @details 检查插件更新、下载和安装
class CUpdateChecker
{
public:
    CUpdateChecker();
    ~CUpdateChecker();

    /// @brief 设置 HTTP 客户端
    void SetHttpClient(std::shared_ptr<Core::IHttpClient> client);

    /// @brief 设置日志回调
    void SetLogCallback(std::function<void(const std::wstring&)> callback);

    /// @brief 检查更新
    /// @param currentVersion 当前版本号
    /// @param info 输出参数，新版本信息
    /// @return true 表示有新版本
    bool CheckForUpdate(const std::wstring& currentVersion, UpdateInfo& info);

    /// @brief 执行更新
    /// @param zipUrl zip 下载地址
    /// @param pluginDir 插件目录
    /// @return true 表示更新成功
    bool PerformUpdate(const std::wstring& zipUrl, const std::wstring& pluginDir);

    /// @brief 检查网络是否可用
    bool IsNetworkAvailable();

    /// @brief 获取插件所在目录
    static std::wstring GetPluginDirectory();

    /// @brief 重启 TrafficMonitor
    static void RestartTrafficMonitor();

    /// @brief 比较版本号
    /// @return <0 表示 v1<v2, 0 表示相等, >0 表示 v1>v2
    static int CompareVersion(const std::wstring& v1, const std::wstring& v2);

private:
    std::shared_ptr<Core::IHttpClient> m_httpClient;
    std::function<void(const std::wstring&)> m_logCallback;

    void Log(const std::wstring& message);
    bool DownloadFile(const std::wstring& url, const std::wstring& savePath);
    bool UnzipFile(const std::wstring& zipPath, const std::wstring& destDir);
};

} // namespace Application
} // namespace StockPlugin
