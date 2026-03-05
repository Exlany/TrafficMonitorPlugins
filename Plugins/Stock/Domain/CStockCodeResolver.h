#pragma once

#include <string>
#include <map>
#include "../StockConstants.h"

namespace StockPlugin {
namespace Domain {

/// @brief 股票代码解析器
/// @details 智能识别和解析股票代码，自动补全前缀
class CStockCodeResolver
{
public:
    CStockCodeResolver();
    ~CStockCodeResolver();

    /// @brief 智能解析股票代码
    /// @param input 用户输入的代码
    /// @return 完整的股票代码（带前缀）
    std::wstring Resolve(const std::wstring& input) const;

    /// @brief 获取股票代码的产品类型
    /// @param code 完整的股票代码
    /// @return 产品类型
    StockConstants::ProductType GetProductType(const std::wstring& code) const;

    /// @brief 获取股票代码的类型索引
    /// @param code 完整的股票代码
    /// @return 类型索引 (0=深证, 1=港股, 2=北交所, 3=上证, 4=美股个股, 5=美股指数, 6=OKX, 7=其他)
    int GetTypeIndex(const std::wstring& code) const;

    /// @brief 智能简称：自动缩短股票名称
    /// @param name 完整的股票名称
    /// @return 简短的名称
    std::wstring SmartShortName(const std::wstring& name) const;

    /// @brief 检查代码是否已有前缀
    bool HasPrefix(const std::wstring& code) const;

    // 所有功能委托给 StockMappings 命名空间实现
    // 无需成员变量
};

} // namespace Domain
} // namespace StockPlugin
