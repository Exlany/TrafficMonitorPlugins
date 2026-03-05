#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace StockPlugin {

// 前向声明
namespace Infrastructure {
    class CWinHttpClient;
}
namespace Domain {
    class CStockRepository;
}

namespace Application {

/// @brief 网络服务
/// @details 负责股票数据的网络请求，从 CDataManager 中分离出来
class CNetworkService
{
public:
    CNetworkService();
    ~CNetworkService();

    /// @brief 设置 HTTP 客户端
    void SetHttpClient(Infrastructure::CWinHttpClient* client);

    /// @brief 设置数据仓库
    void SetRepository(Domain::CStockRepository* repository);

    /// @brief 设置日志路径
    void SetLogPath(const std::wstring& path);

    /// @brief 设置系统代码页
    void SetSystemCodePage(UINT codePage);

    /// @brief 请求实时数据
    /// @param stockCodes 股票代码列表
    void RequestRealtimeData(const std::vector<std::wstring>& stockCodes);

    /// @brief 请求分时数据
    /// @param stockId 股票代码
    /// @param callback 完成回调
    void RequestTimelineData(const std::wstring& stockId, std::function<void()> callback = nullptr);

    /// @brief 请求 OKX 虚拟货币数据
    /// @param code 代码（如 okx_BTC-USDT）
    void RequestOKXData(const std::wstring& code);

private:
    Infrastructure::CWinHttpClient* m_httpClient{nullptr};
    Domain::CStockRepository* m_repository{nullptr};
    std::wstring m_logPath;
    UINT m_systemCodePage{936};

    void Log(const std::wstring& message);
};

} // namespace Application
} // namespace StockPlugin
