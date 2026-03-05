#pragma once

#include <string>
#include <map>
#include <vector>
#include <utility>
#include "StockConstants.h"

// ============================================================================
// StockMappings 命名空间 - 股票映射表和数据
// ============================================================================
// 这是股票代码识别和名称映射的唯一数据源
// 所有映射表使用 static local 模式，保证线程安全初始化
// ============================================================================

namespace StockMappings
{
    // ========================================================================
    // 数据获取函数 (返回 const 引用，避免拷贝)
    // ========================================================================

    // 股票名称 -> 简称 的直接映射表
    const std::map<std::wstring, std::wstring>& GetNameDirectMap();

    // 股票名称常见前缀列表 (如 "中国", "上海" 等)
    const std::vector<std::wstring>& GetNamePrefixes();

    // 股票名称常见后缀列表 (如 "股份有限公司", "集团" 等)
    const std::vector<std::wstring>& GetNameSuffixes();

    // 基金公司名称列表
    const std::vector<std::wstring>& GetFundCompanies();

    // 基金后缀列表 (如 "联接A", "ETF联接" 等)
    const std::vector<std::wstring>& GetFundSuffixes();

    // 商品关键词列表 (如 "黄金", "原油" 等)
    const std::vector<std::pair<std::wstring, std::wstring>>& GetCommodityKeywords();

    // 美股指数/期货映射表 (如 "nasdaq" -> "int_nasdaq")
    const std::map<std::wstring, std::wstring>& GetUSIndexMap();

    // 美股个股代码映射表 (如 "aapl" -> "gb_aapl")
    const std::map<std::wstring, std::wstring>& GetUSStockMap();

    // 虚拟货币映射表 (预留)
    const std::map<std::wstring, std::wstring>& GetCryptoMap();

    // 外汇映射表 (预留)
    const std::map<std::wstring, std::wstring>& GetForexMap();

    // ========================================================================
    // 智能识别函数
    // ========================================================================

    // 智能生成股票简称
    // 例如: "贵州茅台" -> "茅台", "招商银行" -> "招行"
    std::wstring SmartShortName(const std::wstring& name);

    // 智能识别股票代码并添加市场前缀
    // 例如: "603986" -> "sh603986", "AAPL" -> "gb_aapl"
    std::wstring SmartStockCode(const std::wstring& input);

    // 获取股票类型索引 (用于UI显示)
    // 返回: 0=深证, 1=港股, 2=北交所, 3=上证, 4=美股, 5=指数/期货, 6=OKX虚拟货币, 7=其他
    int GetStockTypeIndex(const std::wstring& code);

    // 获取产品类型枚举
    StockConstants::ProductType GetProductType(const std::wstring& code);

} // namespace StockMappings
