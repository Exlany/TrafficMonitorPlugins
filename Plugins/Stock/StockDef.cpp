#include "pch.h"
#include "StockDef.h"
#include <iomanip>
#include "Common.h"
#include <DataManager.h>
#include <Stock.h>
#include "utilities/yyjson/yyjson.h"
#include "utilities/Common.h"
#include "utilities/JsonHelper.h"

// 美股数据长度
constexpr auto _DATA_LEN_MG = 36;
// 上证A股 & 指数数据长度
constexpr auto _DATA_LEN_SH = 34;
// 深证A股 & 指数数据长度
constexpr auto _DATA_LEN_SZ = 33;
// 北证A股 & 指数数据长度
constexpr auto _DATA_LEN_BJ = 39;
// 港股数据长度
constexpr auto _DATA_LEN_HK = 25;

using namespace STOCK;

void STOCK::StockMarket::LoadRealtimeDataByJson(std::string json)
{
  std::lock_guard<std::mutex> lock(Stock::Instance().m_stockDataMutex);
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

void STOCK::RealTimeData::Load(std::wstring key, std::vector<std::string> data_arr)
{
  size_t data_size = static_cast<size_t>(data_arr.size());

  if (key.find(kMG) == 0)
  {
    LoadMG(data_arr, data_size);
  }
  else if (key.find(kSH) == 0)
  {
    LoadSH(data_arr, data_size);
  }
  else if (key.find(kSZ) == 0)
  {
    LoadSZ(data_arr, data_size);
  }
  else if (key.find(kBJ) == 0)
  {
    LoadBJ(data_arr, data_size);
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
    char buff[32];
    if (g_data.m_setting_data.m_price_decimal == 2)
      sprintf_s(buff, "%.2f", currentPrice);
    else
      sprintf_s(buff, "%.3f", currentPrice);
    displayPrice = CCommon::StrToUnicode(buff);

    sprintf_s(buff, "%+.2f%%", ((currentPrice - prevClosePrice) / prevClosePrice * 100));
    displayFluctuation = CCommon::StrToUnicode(buff);
  }
}

void STOCK::RealTimeData::LoadMG(std::vector<std::string> data, size_t size)
{
  if (size < _DATA_LEN_MG)
  {
    return;
  }
  openPrice = {convert<Price>(data[5])};
  prevClosePrice = {convert<Price>(data[26])};
  currentPrice = {convert<Price>(data[1])};
  highPrice = {convert<Price>(data[6])};
  lowPrice = {convert<Price>(data[7])};
  volume = {convert<Volume>(data[10])};
  // turnover = {convert<Price>(data[5])};
}

void STOCK::RealTimeData::LoadAG(std::vector<std::string> data, size_t size)
{
  openPrice = {convert<Price>(data[1])};
  prevClosePrice = {convert<Price>(data[2])};
  currentPrice = {convert<Price>(data[3])};
  highPrice = {convert<Price>(data[4])};
  lowPrice = {convert<Price>(data[5])};
  volume = {convert<Volume>(data[8])};
  turnover = {convert<Amount>(data[9])};

  Price upperLimit = abs(highPrice - prevClosePrice);
  Price lowerLimit = abs(lowPrice - prevClosePrice);

  priceLimit = max(upperLimit, lowerLimit);

  // 设置买卖盘数据
  askLevels[4] = {{convert<Price>(data[29])}, {convert<Volume>(data[28])}};
  askLevels[3] = {{convert<Price>(data[27])}, {convert<Volume>(data[26])}};
  askLevels[2] = {{convert<Price>(data[25])}, {convert<Volume>(data[24])}};
  askLevels[1] = {{convert<Price>(data[23])}, {convert<Volume>(data[22])}};
  askLevels[0] = {{convert<Price>(data[21])}, {convert<Volume>(data[20])}};

  bidLevels[0] = {{convert<Price>(data[11])}, {convert<Volume>(data[10])}};
  bidLevels[1] = {{convert<Price>(data[13])}, {convert<Volume>(data[12])}};
  bidLevels[2] = {{convert<Price>(data[15])}, {convert<Volume>(data[14])}};
  bidLevels[3] = {{convert<Price>(data[17])}, {convert<Volume>(data[16])}};
  bidLevels[4] = {{convert<Price>(data[19])}, {convert<Volume>(data[18])}};
}

void STOCK::RealTimeData::LoadSH(std::vector<std::string> data, size_t size)
{
  if (size < _DATA_LEN_SH)
  {
    return;
  }
  LoadAG(data, size);
}

void STOCK::RealTimeData::LoadSZ(std::vector<std::string> data, size_t size)
{
  if (size < _DATA_LEN_SZ)
  {
    return;
  }
  LoadAG(data, size);
}

void STOCK::RealTimeData::LoadBJ(std::vector<std::string> data, size_t size)
{
  if (size < _DATA_LEN_BJ)
  {
    return;
  }
  LoadAG(data, size);
}

void STOCK::RealTimeData::LoadHK(std::vector<std::string> data, size_t size)
{
  if (size < _DATA_LEN_HK)
  {
    return;
  }
  openPrice = {convert<Price>(data[2])};
  prevClosePrice = {convert<Price>(data[3])};
  currentPrice = {convert<Price>(data[6])};
  highPrice = {convert<Price>(data[4])};
  lowPrice = {convert<Price>(data[5])};
  volume = {convert<Volume>(data[12])};
  turnover = {convert<Price>(data[11])};
}

void STOCK::RealTimeData::LoadINT(std::vector<std::string> data, size_t size)
{
  // 国际指数/期货格式: 名称,当前价,涨跌点数,涨跌幅%
  if (size < 4)
  {
    return;
  }
  currentPrice = {convert<Price>(data[1])};
  Price changePoints = {convert<Price>(data[2])};
  // 通过当前价和涨跌点数反推昨收价
  prevClosePrice = currentPrice - changePoints;
  openPrice = prevClosePrice;
  highPrice = currentPrice;
  lowPrice = currentPrice;
}

void STOCK::StockMarket::LoadTimelineDataByJson(std::wstring stock_id, CString *pData)
{
  auto data = g_data.GetStockData(stock_id);
  {
    std::lock_guard<std::mutex> lock(Stock::Instance().m_stockDataMutex);
    data->clearTimelinePoint();
    if (pData)
    {
      data->addTimelinePoint(*pData);
    }
  }
  Stock::Instance().UpdateKLine();
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
  auto it = g_data.m_setting_data.m_stock_aliases.find(info.code);
  if (it != g_data.m_setting_data.m_stock_aliases.end() && !it->second.empty())
  {
    return it->second;
  }
  // 其次使用智能简称
  return CCommon::SmartShortName(info.displayName);
}

static Volume GetJsonVolume(yyjson_val *obj, const char *key)
{
  if (obj != nullptr)
  {
    yyjson_val *val = yyjson_obj_get(obj, key);
    try
    {

      if (val != nullptr)
      {
        if (yyjson_is_uint(val))
        {
          return static_cast<Volume>(yyjson_get_uint(val));
        }
        else if (yyjson_is_str(val))
        {
          return static_cast<Volume>(std::stoul(yyjson_get_str(val)));
        }
      }
    }
    catch (const std::exception &e)
    {
      return 0L;
    }
  }
  return 0L;
}

static Price GetJsonPrice(yyjson_val *obj, const char *key)
{
  if (obj != nullptr)
  {
    yyjson_val *val = yyjson_obj_get(obj, key);
    try
    {
      if (val != nullptr)
      {
        if (yyjson_is_real(val))
        {
          return static_cast<Price>(yyjson_get_real(val));
        }
        else if (yyjson_is_sint(val))
        {
          return static_cast<Price>(yyjson_get_sint(val));
        }
        else if (yyjson_is_uint(val))
        {
          return static_cast<Price>(yyjson_get_uint(val));
        }
        else if (yyjson_is_str(val))
        {
          return static_cast<Price>(std::stod(yyjson_get_str(val)));
        }
      }
    }
    catch (const std::invalid_argument &e)
    {
      return 0.0F;
    }
    catch (const std::out_of_range &e)
    {
      return 0.0F;
    }
  }
  return 0.0F;
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
          point.volume = GetJsonVolume(item, "v");
          point.price = GetJsonPrice(item, "p");
          point.averagePrice = GetJsonPrice(item, "avg_p");
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
      CCommon::WriteLog("OKX API error!", g_data.m_log_path.c_str());
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

  std::lock_guard<std::mutex> lock(Stock::Instance().m_stockDataMutex);
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
  stockData->realTimeData.currentPrice = GetJsonPrice(data, "last");
  stockData->realTimeData.prevClosePrice = GetJsonPrice(data, "open24h");
  stockData->realTimeData.openPrice = GetJsonPrice(data, "open24h");
  stockData->realTimeData.highPrice = GetJsonPrice(data, "high24h");
  stockData->realTimeData.lowPrice = GetJsonPrice(data, "low24h");
  stockData->realTimeData.volume = GetJsonVolume(data, "vol24h");

  // 计算显示价格和涨跌幅
  Price currentPrice = stockData->realTimeData.currentPrice;
  Price prevClosePrice = stockData->realTimeData.prevClosePrice;

  if (currentPrice > 0 && prevClosePrice > 0)
  {
    char buff[32];
    // 虚拟货币价格自适应显示：
    // - 十万级别以上(>=100000): 整数显示
    // - 万级别(>=10000): 1位小数
    // - 千级别(>=1000): 2位小数
    // - 百级别(>=100): 2位小数
    // - 十级别(>=10): 3位小数
    // - 个位级别(>=1): 4位小数
    // - 小数级别(>=0.01): 6位小数
    // - 极小值(<0.01): 8位小数
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
    stockData->realTimeData.displayPrice = CCommon::StrToUnicode(buff);

    sprintf_s(buff, "%+.2f%%", ((currentPrice - prevClosePrice) / prevClosePrice * 100));
    stockData->realTimeData.displayFluctuation = CCommon::StrToUnicode(buff);
  }

  yyjson_doc_free(doc);
}
