#pragma once

#include <string>
#include <vector>
#include "../StockDef.h"

namespace StockPlugin {
namespace Domain {

/// @brief 股票数据解析器
/// @details 负责解析各种数据源返回的股票数据
class CStockDataParser
{
public:
    CStockDataParser();
    ~CStockDataParser();

    /// @brief 设置系统代码页
    void SetSystemCodePage(UINT codePage);

    /// @brief 解析新浪实时数据
    /// @param data 新浪 API 返回的数据（格式: var hq_str_xxx="..."）
    /// @param market 股票市场对象
    /// @note 数据格式为新浪特有格式，非标准 JSON
    void ParseSinaRealtimeData(const std::string& data, STOCK::StockMarket& market);

    /// @brief 解析新浪分时数据
    /// @param stockId 股票代码
    /// @param data 新浪 API 返回的分时数据
    /// @param market 股票市场对象
    /// @note 数据格式为新浪特有格式
    void ParseSinaTimelineData(const std::wstring& stockId, const std::string& data, STOCK::StockMarket& market);

    /// @brief 解析 OKX 虚拟货币数据
    /// @param code 代码（如 okx_BTC-USDT）
    /// @param json OKX API 返回的 JSON 数据
    /// @param market 股票市场对象
    void ParseOKXData(const std::wstring& code, const std::string& json, STOCK::StockMarket& market);

    /// @brief 安全的字符串转 double
    static double SafeStod(const std::string& s);

    /// @brief 安全的字符串转 long long
    static long long SafeStoll(const std::string& s);

private:
    UINT m_systemCodePage{936};
};

} // namespace Domain
} // namespace StockPlugin
