#include "pch.h"
#include "CStockDataParser.h"
#include "../Utils/StringUtils.h"
#include "../StockConstants.h"
#include "utilities/yyjson/yyjson.h"

namespace StockPlugin {
namespace Domain {

CStockDataParser::CStockDataParser() = default;

CStockDataParser::~CStockDataParser() = default;

void CStockDataParser::SetSystemCodePage(UINT codePage)
{
    m_systemCodePage = codePage;
}

double CStockDataParser::SafeStod(const std::string& s)
{
    try { return std::stod(s); }
    catch (...) { return 0.0; }
}

long long CStockDataParser::SafeStoll(const std::string& s)
{
    try { return std::stoll(s); }
    catch (...) { return 0; }
}

void CStockDataParser::ParseSinaRealtimeData(const std::string& data, STOCK::StockMarket& market)
{
    // 注意：新浪返回的数据格式为 "var hq_str_xxx=..."，非标准 JSON
    if (data.empty())
        return;

    // 只清除普通股票数据，不清除 OKX 数据
    market.ClearRealtimeData(false);

    // 分割行
    std::vector<std::string> lines = Utils::StringUtils::Split(
        Utils::StringUtils::RemoveChar(data, '\n'), ";");

    if (lines.empty())
        return;

    bool isUtf8 = (m_systemCodePage == 65001);

    for (const std::string& line : lines)
    {
        if (line.empty())
            continue;

        // 移除 "var hq_str_" 前缀和引号
        std::string cleanLine = Utils::StringUtils::RemoveSubstr(line, "var hq_str_");
        cleanLine = Utils::StringUtils::RemoveChar(cleanLine, '\"');

        // 分割 key=value
        std::vector<std::string> itemArr = Utils::StringUtils::Split(cleanLine, '=');
        if (itemArr.empty())
            continue;

        std::wstring key = Utils::StringUtils::ToUnicode(itemArr[0], isUtf8);
        auto stockData = market.getStock(key);
        stockData->info.code = key;
        stockData->info.is_ok = (itemArr.size() >= 2);

        if (!stockData->info.is_ok)
            continue;

        // 分割数据字段
        std::vector<std::string> dataArr = Utils::StringUtils::Split(itemArr[1], ',');
        if (dataArr.empty())
            continue;

        stockData->info.displayName = Utils::StringUtils::ToUnicode(dataArr[0], isUtf8);
        stockData->realTimeData.Load(key, dataArr);
    }
}

void CStockDataParser::ParseSinaTimelineData(const std::wstring& stockId, const std::string& rawData, STOCK::StockMarket& market)
{
    // 注意：新浪分时数据格式为特有格式，解析逻辑在 StockData::addTimelinePoint 中
    auto stockData = market.getStock(stockId);
    if (!stockData)
        return;

    stockData->clearTimelinePoint();

    if (rawData.empty())
        return;

    CString strData(rawData.c_str());
    stockData->addTimelinePoint(strData);
}

void CStockDataParser::ParseOKXData(const std::wstring& code, const std::string& json, STOCK::StockMarket& market)
{
    // 委托给 StockMarket 的现有实现
    market.LoadOKXDataByJson(code, json);
}

} // namespace Domain
} // namespace StockPlugin
