#include "pch.h"
#include "StockDef.h"
#include "StockConstants.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include "Common.h"
#include <DataManager.h>
#include "utilities/yyjson/yyjson.h"
#include "utilities/Common.h"
#include "utilities/JsonHelper.h"

using namespace STOCK;
using namespace StockConstants;

static Price SafeStod(const std::string& s)
{
    try { return std::stod(s); }
    catch (...) { return 0.0; }
}

static Volume SafeStoll(const std::string& s)
{
    try { return std::stoll(s); }
    catch (...) { return 0; }
}

void STOCK::StockMarket::LoadRealtimeDataByJson(std::string json)
{
  std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());
  // 只清除普通股票数据，不清除OKX数据
  ClearRealtimeData(false);

  if (json == "")
  {
    CCommon::WriteLog("Response is EMPTY!", g_data.m_log_path.c_str());
    return;
  }

  std::vector<std::string> lines = CCommon::split(CCommon::removeChar(json, '\n'), ";");
  if (lines.size() < 1)
  {
    CCommon::WriteLog("json is INVALID!", g_data.m_log_path.c_str());
    return;
  }

  for (std::string line : lines)
  {
    if (line.empty())
    {
      continue;
    }
    line = CCommon::removeChar(CCommon::removeStr(line, "var hq_str_"), '\"');

    std::vector<std::string> item_arr = CCommon::split(line, '=');
    if (item_arr.size() <= 0)
    {
      CCommon::WriteLog("json is INVALID!", g_data.m_log_path.c_str());
      continue;
    }

    // 根据系统代码页确定是否为UTF-8
    bool is_utf8 = (g_data.m_system_code_page == 65001);
    std::wstring key = CCommon::StrToUnicode(item_arr[0].c_str(), is_utf8);
    auto stockData = getStock(key);
    stockData->info.code = CCommon::StrToUnicode(item_arr[0].c_str(), is_utf8);

    stockData->info.is_ok = item_arr.size() >= 2;

    if (!stockData->info.is_ok)
    {
      CCommon::WriteLog("json is INVALID!", g_data.m_log_path.c_str());
      continue;
    }

    std::string data = item_arr[1];

    std::vector<std::string> data_arr = CCommon::split(data, ",");

    stockData->info.displayName = CCommon::StrToUnicode(data_arr[0].c_str(), is_utf8);

    stockData->realTimeData.Load(key, data_arr);
  }
}

void STOCK::RealTimeData::Load(std::wstring key, const std::vector<std::string>& data_arr)
{
  size_t data_size = static_cast<size_t>(data_arr.size());

  if (key.find(kMG) == 0)
  {
    LoadMG(data_arr, data_size);
  }
  else if (key.find(kSH) == 0)
  {
    if (data_size >= DATA_LEN_SH)
      LoadAG(data_arr, data_size);
  }
  else if (key.find(kSZ) == 0)
  {
    if (data_size >= DATA_LEN_SZ)
      LoadAG(data_arr, data_size);
  }
  else if (key.find(kBJ) == 0)
  {
    if (data_size >= DATA_LEN_BJ)
      LoadAG(data_arr, data_size);
  }
  else if (key.find(kHK) == 0)
  {
    LoadHK(data_arr, data_size);
  }
  else if (key.find(kMGI) == 0)
  {
    LoadINT(data_arr, data_size);
  }

  if (currentPrice > 0 && prevClosePrice > 0)
  {
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    FormatDisplay(settings.priceDecimal);
  }
}

void STOCK::RealTimeData::LoadMG(const std::vector<std::string>& data, size_t size)
{
  if (size < DATA_LEN_MG)
  {
    return;
  }
  openPrice = {SafeStod(data[5])};
  prevClosePrice = {SafeStod(data[26])};
  currentPrice = {SafeStod(data[1])};
  highPrice = {SafeStod(data[6])};
  lowPrice = {SafeStod(data[7])};
  volume = {SafeStoll(data[10])};
  // turnover = {SafeStod(data[5])};
}

void STOCK::RealTimeData::LoadAG(const std::vector<std::string>& data, size_t size)
{
  if (size < 30)
    return;

  openPrice = {SafeStod(data[1])};
  prevClosePrice = {SafeStod(data[2])};
  currentPrice = {SafeStod(data[3])};
  highPrice = {SafeStod(data[4])};
  lowPrice = {SafeStod(data[5])};
  volume = {SafeStoll(data[8])};
  turnover = {SafeStod(data[9])};

  Price upperLimit = std::abs(highPrice - prevClosePrice);
  Price lowerLimit = std::abs(lowPrice - prevClosePrice);

  priceLimit = (std::max)(upperLimit, lowerLimit);

  // 设置买卖盘数据
  askLevels[4] = {{SafeStod(data[29])}, {SafeStoll(data[28])}};
  askLevels[3] = {{SafeStod(data[27])}, {SafeStoll(data[26])}};
  askLevels[2] = {{SafeStod(data[25])}, {SafeStoll(data[24])}};
  askLevels[1] = {{SafeStod(data[23])}, {SafeStoll(data[22])}};
  askLevels[0] = {{SafeStod(data[21])}, {SafeStoll(data[20])}};

  bidLevels[0] = {{SafeStod(data[11])}, {SafeStoll(data[10])}};
  bidLevels[1] = {{SafeStod(data[13])}, {SafeStoll(data[12])}};
  bidLevels[2] = {{SafeStod(data[15])}, {SafeStoll(data[14])}};
  bidLevels[3] = {{SafeStod(data[17])}, {SafeStoll(data[16])}};
  bidLevels[4] = {{SafeStod(data[19])}, {SafeStoll(data[18])}};
}

void STOCK::RealTimeData::LoadHK(const std::vector<std::string>& data, size_t size)
{
  if (size < DATA_LEN_HK)
  {
    return;
  }
  openPrice = {SafeStod(data[2])};
  prevClosePrice = {SafeStod(data[3])};
  currentPrice = {SafeStod(data[6])};
  highPrice = {SafeStod(data[4])};
  lowPrice = {SafeStod(data[5])};
  volume = {SafeStoll(data[12])};
  turnover = {SafeStod(data[11])};
}

void STOCK::RealTimeData::LoadINT(const std::vector<std::string>& data, size_t size)
{
  // 国际指数/期货格式: 名称,当前价,涨跌点数,涨跌幅%
  if (size < 4)
  {
    return;
  }
  currentPrice = {SafeStod(data[1])};
  Price changePoints = {SafeStod(data[2])};
  // 通过当前价和涨跌点数反推昨收价
  prevClosePrice = currentPrice - changePoints;
  openPrice = prevClosePrice;
  highPrice = currentPrice;
  lowPrice = currentPrice;
}

void STOCK::RealTimeData::FormatDisplay(int priceDecimal, bool adaptiveDecimal)
{
  if (currentPrice <= 0 || prevClosePrice <= 0)
    return;

  char buff[32];
  if (adaptiveDecimal)
  {
    if (currentPrice >= 100000)
      sprintf_s(buff, "%.0f", currentPrice);
    else if (currentPrice >= 10000)
      sprintf_s(buff, "%.1f", currentPrice);
    else if (currentPrice >= 100)
      sprintf_s(buff, "%.2f", currentPrice);
    else if (currentPrice >= 10)
      sprintf_s(buff, "%.3f", currentPrice);
    else if (currentPrice >= 1)
      sprintf_s(buff, "%.4f", currentPrice);
    else if (currentPrice >= 0.01)
      sprintf_s(buff, "%.6f", currentPrice);
    else
      sprintf_s(buff, "%.8f", currentPrice);
  }
  else
  {
    if (priceDecimal == 2)
      sprintf_s(buff, "%.2f", currentPrice);
    else
      sprintf_s(buff, "%.3f", currentPrice);
  }
  displayPrice = CCommon::StrToUnicode(buff);

  sprintf_s(buff, "%+.2f%%", ((currentPrice - prevClosePrice) / prevClosePrice * 100));
  displayFluctuation = CCommon::StrToUnicode(buff);
}

void STOCK::StockMarket::LoadTimelineDataByJson(std::wstring stock_id, CString *pData)
{
  std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());
  auto data = getStock(stock_id);
  if (data == nullptr)
    return;
  data->clearTimelinePoint();
  if (pData)
  {
    data->addTimelinePoint(*pData);
  }
}

std::wstring STOCK::StockData::GetCurrentDisplay(bool include_name) const
{
  std::wstringstream wss;
  if (info.is_ok)
  {
    if (include_name)
      wss << GetDisplayName() << ": ";
    wss << realTimeData.displayPrice << ' ' << realTimeData.displayFluctuation;
  }
  else
  {
    wss << info.code + L" " + g_data.StringRes(IDS_LOAD_FAIL).GetString();
  }
  return wss.str();
}

std::wstring STOCK::StockData::GetDisplayName() const
{
  // 优先使用自定义别名
  SettingsSnapshot settings = g_data.GetSettingsSnapshot();
  auto it = settings.aliases.find(info.code);
  if (it != settings.aliases.end() && !it->second.empty())
  {
    return it->second;
  }
  // 其次使用智能简称
  return CCommon::SmartShortName(info.displayName);
}

template<typename T>
static T GetJsonNumeric(yyjson_val *obj, const char *key)
{
  if (obj == nullptr)
    return T{};

  yyjson_val *val = yyjson_obj_get(obj, key);
  if (val == nullptr)
    return T{};

  try
  {
    if (yyjson_is_real(val))
      return static_cast<T>(yyjson_get_real(val));
    if (yyjson_is_sint(val))
      return static_cast<T>(yyjson_get_sint(val));
    if (yyjson_is_uint(val))
      return static_cast<T>(yyjson_get_uint(val));
    if (yyjson_is_str(val))
      return static_cast<T>(std::stod(yyjson_get_str(val)));
  }
  catch (const std::exception &)
  {
    return T{};
  }
  return T{};
}

void STOCK::StockData::addTimelinePoint(const CString &json_data)
{
  std::string _json_data = CCommon::UnicodeToStr(json_data);
  yyjson_doc *doc = yyjson_read(_json_data.c_str(), _json_data.size(), 0);
  if (doc != nullptr)
  {
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (root == nullptr)
    {
      yyjson_doc_free(doc);
      return;
    }

    yyjson_val *result = yyjson_obj_get(root, "result");
    if (result == nullptr)
    {
      yyjson_doc_free(doc);
      return;
    }

    yyjson_val *data = yyjson_obj_get(result, "data");
    if (data != nullptr && yyjson_is_arr(data))
    {
      yyjson_val *item;
      yyjson_arr_iter iter;
      yyjson_arr_iter_init(data, &iter);
      while ((item = yyjson_arr_iter_next(&iter)))
      {
        if (item != nullptr)
        {
          TimelinePoint point = TimelinePoint();
          point.time = utilities::JsonHelper::GetJsonString(item, "m");
          point.volume = GetJsonNumeric<Volume>(item, "v");
          point.price = GetJsonNumeric<Price>(item, "p");
          point.averagePrice = GetJsonNumeric<Price>(item, "avg_p");
          addTimelinePoint(point);
        }
      }
    }
    yyjson_doc_free(doc);
  }
}

void STOCK::StockMarket::LoadOKXDataByJson(const std::wstring& code, const std::string& json)
{
  if (json.empty())
  {
    CCommon::WriteLog("OKX Response is EMPTY!", g_data.m_log_path.c_str());
    return;
  }

  yyjson_doc *doc = yyjson_read(json.c_str(), json.size(), 0);
  if (doc == nullptr)
  {
    CCommon::WriteLog("OKX JSON parse failed!", g_data.m_log_path.c_str());
    return;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (root == nullptr)
  {
    yyjson_doc_free(doc);
    return;
  }

  // 检查返回码
  yyjson_val *codeVal = yyjson_obj_get(root, "code");
  if (codeVal != nullptr && yyjson_is_str(codeVal))
  {
    const char* codeStr = yyjson_get_str(codeVal);
    if (strcmp(codeStr, "0") != 0)
    {
      yyjson_val *msgVal = yyjson_obj_get(root, "msg");
      std::string errMsg = "OKX API error: code=";
      errMsg += codeStr;
      if (msgVal != nullptr && yyjson_is_str(msgVal))
      {
        errMsg += ", msg=";
        errMsg += yyjson_get_str(msgVal);
      }
      CCommon::WriteLog(errMsg.c_str(), g_data.m_log_path.c_str());
      yyjson_doc_free(doc);
      return;
    }
  }

  yyjson_val *dataArr = yyjson_obj_get(root, "data");
  if (dataArr == nullptr || !yyjson_is_arr(dataArr))
  {
    yyjson_doc_free(doc);
    return;
  }

  yyjson_val *data = yyjson_arr_get_first(dataArr);
  if (data == nullptr)
  {
    yyjson_doc_free(doc);
    return;
  }

  std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());
  auto stockData = getStock(code);
  stockData->info.code = code;
  stockData->info.is_ok = true;

  // 从 okx_BTC-USDT 提取显示名称 BTC-USDT
  std::wstring instId = code.substr(4);
  // 简化显示名称，如 BTC-USDT -> BTC
  size_t dashPos = instId.find(L'-');
  if (dashPos != std::wstring::npos)
  {
    stockData->info.displayName = instId.substr(0, dashPos);
  }
  else
  {
    stockData->info.displayName = instId;
  }

  // 解析价格数据
  // last: 最新成交价
  // open24h: 24小时开盘价（作为昨收价）
  // high24h: 24小时最高价
  // low24h: 24小时最低价
  // vol24h: 24小时成交量
  stockData->realTimeData.currentPrice = GetJsonNumeric<Price>(data, "last");
  stockData->realTimeData.prevClosePrice = GetJsonNumeric<Price>(data, "open24h");
  stockData->realTimeData.openPrice = GetJsonNumeric<Price>(data, "open24h");
  stockData->realTimeData.highPrice = GetJsonNumeric<Price>(data, "high24h");
  stockData->realTimeData.lowPrice = GetJsonNumeric<Price>(data, "low24h");
  stockData->realTimeData.volume = GetJsonNumeric<Volume>(data, "vol24h");

  stockData->realTimeData.FormatDisplay(0, true);

  yyjson_doc_free(doc);
}
