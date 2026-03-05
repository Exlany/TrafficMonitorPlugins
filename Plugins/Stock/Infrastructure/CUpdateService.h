#pragma once

#include <string>
#include <functional>

namespace StockPlugin {
namespace Infrastructure {

/// @brief 更新信息结构
struct UpdateInfo
{
    std::wstring version;           // 新版本号
    std::wstring downloadUrl;       // zip 下载地址
    std::wstring releasePageUrl;    // Release 页面地址
    std::wstring releaseNotes;      // 更新说明
};

/// @brief 更新服务
/// @details 负责检查更新、下载和安装插件更新
class CUpdateService
{
public:
    CUpdateService();
    ~CUpdateService();

    /// @brief 设置日志回调
    void SetLogCallback(std::function<void(const std::wstring&)> callback);

    /// @brief 检查插件更新
    /// @param currentVersion 当前版本号
    /// @param info 输出参数，新版本信息
    /// @return true 表示有新版本
    bool CheckForUpdate(const std::wstring& currentVersion, UpdateInfo& info);

    /// @brief 执行自动更新
    /// @param zipUrl zip 下载地址
    /// @param pluginDir 插件目录
    /// @return true 表示更新成功（需要重启生效）
    bool PerformUpdate(const std::wstring& zipUrl, const std::wstring& pluginDir);

    /// @brief 下载文件到指定路径
    bool DownloadFile(const std::wstring& url, const std::wstring& savePath);

    /// @brief 解压 zip 文件到指定目录
    bool UnzipFile(const std::wstring& zipPath, const std::wstring& destDir);

    /// @brief 获取插件所在目录
    static std::wstring GetPluginDirectory();

    /// @brief 重启 TrafficMonitor
    static void RestartTrafficMonitor();

    /// @brief 比较版本号
    /// @return <0 表示 v1<v2, 0 表示相等, >0 表示 v1>v2
    static int CompareVersion(const std::wstring& v1, const std::wstring& v2);

private:
    void Log(const std::wstring& message);

    std::function<void(const std::wstring&)> m_logCallback;
};

} // namespace Infrastructure
} // namespace StockPlugin
