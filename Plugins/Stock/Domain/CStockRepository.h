#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include "../Core/IHttpClient.h"
#include "../StockDef.h"

namespace StockPlugin {
namespace Domain {

/// @brief 股票数据仓库
/// @details 封装股票数据的存储和访问
/// @note 外部通过 GetMutex() 获取互斥锁进行同步
///       内部不自行加锁，由调用者负责（因为 LoadRealtimeDataByJson 等方法内部会加锁）
class CStockRepository
{
public:
    CStockRepository();
    ~CStockRepository();

    /// @brief 获取股票数据
    std::shared_ptr<STOCK::StockData> GetStockData(const std::wstring& code);

    /// @brief 清除实时数据
    void ClearRealtimeData(bool includeOKX = true);

    /// @brief 获取数据互斥锁（用于外部同步）
    std::mutex& GetMutex() { return m_mutex; }

    /// @brief 获取内部 StockMarket 引用（用于数据加载）
    STOCK::StockMarket& GetMarket() { return m_market; }

private:
    STOCK::StockMarket m_market;
    std::mutex m_mutex;
};

} // namespace Domain
} // namespace StockPlugin
