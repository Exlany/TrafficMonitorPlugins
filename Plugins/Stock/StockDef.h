#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <iostream>

namespace STOCK
{

  // 价格
  using Price = double;
  // 数量
  using Volume = long long;
  // 金额
  using Amount = double;
  using TimePoint = std::string;

  // 买卖盘信息
  struct OrderLevel
  {
    Price price;   // 价格
    Volume volume; // 数量

    OrderLevel() : price(0.0), volume(0) {}
    OrderLevel(Price p, Volume v) : price(p), volume(v) {}
  };

  // 最新交易数据
  struct RealTimeData
  {
    Price openPrice;      // 今日开盘价
    Price prevClosePrice; // 昨日收盘价
    Price currentPrice;   // 当前价格
    Price highPrice;      // 最高价
    Price lowPrice;       // 最低价
    Volume volume;        // 成交量(股)
    Amount turnover;      // 成交额(元)

    std::wstring displayPrice = L"--";
    std::wstring displayFluctuation = L"--%";

    Price priceLimit; // 价格限制

    // 买卖盘口，使用数组便于遍历
    static const int MAX_LEVEL = 5;

    OrderLevel askLevels[MAX_LEVEL]; // 卖盘(5档)
    OrderLevel bidLevels[MAX_LEVEL]; // 买盘(5档)

    RealTimeData() : openPrice(0.0),
                     prevClosePrice(0.0),
                     currentPrice(0.0),
                     highPrice(0.0),
                     lowPrice(0.0),
                     volume(0),
                     turnover(0.0),
                     priceLimit(0.0)
    {
    }

    void Load(std::wstring key, const std::vector<std::string>& data);
    void LoadMG(const std::vector<std::string>& data, size_t size);
    void LoadAG(const std::vector<std::string>& data, size_t size);
    void LoadHK(const std::vector<std::string>& data, size_t size);
    void LoadINT(const std::vector<std::string>& data, size_t size);  // 国际指数/期货
    // 格式化显示价格和涨跌幅
    // adaptiveDecimal: true 表示根据价格大小自适应小数位（用于虚拟货币）
    void FormatDisplay(int priceDecimal, bool adaptiveDecimal = false);
    bool HasValidQuote() const;
    double GetChangePercent() const;
  };

  // 分时数据点
  struct TimelinePoint
  {
    TimePoint time;     // 时间点
    Volume volume;      // 成交量
    Price price;        // 价格
    Price averagePrice; // 均价

    TimelinePoint() : volume(0), price(0.0), averagePrice(0.0) {}
    TimelinePoint(const TimePoint &time, Volume vol, Price p, Price avg) : time(time), volume(vol), price(p), averagePrice(avg) {}
  };

  // 定义不同的数据周期
  enum class Period
  {
    TIMELINE, // 分时
    MIN1,     // 1分钟
    MIN5,     // 5分钟
    MIN15,    // 15分钟
    MIN30,    // 30分钟
    HOUR1,    // 1小时
    DAY,      // 日线
    WEEK,     // 周线
    MONTH,    // 月线
    YEAR      // 年线
  };

  // 历史数据基类
  class HistoricalDataBase
  {
  public:
    virtual ~HistoricalDataBase() = default;
    virtual Period GetPeriod() const = 0;
    virtual TimePoint GetStartTime() const = 0;
    virtual TimePoint GetEndTime() const = 0;
  };

  // 分时历史数据
  class TimelineData : public HistoricalDataBase
  {
  public:
    std::vector<TimelinePoint> data;
    TimelineData() {};
    Period GetPeriod() const override { return Period::TIMELINE; }
    TimePoint GetStartTime() const { return data.empty() ? TimePoint() : data.front().time; }
    TimePoint GetEndTime() const { return data.empty() ? TimePoint() : data.back().time; }
    void Clear() { data.clear(); }
  };

  // 股票基础信息
  struct StockInfo
  {
    std::wstring code;              // 股票代码
    std::wstring displayName = L""; // 股票名称

    bool is_ok = true;              // 加载成功标志
  };

  // 股票数据结构
  class StockData
  {
  public:
    StockInfo info;            // 基础信息
    RealTimeData realTimeData; // 最新数据

    std::wstring GetCurrentDisplay(bool include_name = true) const;
    // 获取显示名称（优先别名，其次智能简称）
    std::wstring GetDisplayName() const;
    // 获取带轻量状态前缀的显示名称
    std::wstring GetDisplayNameWithStatus() const;

    // 使用智能指针管理历史数据
    std::map<Period, std::shared_ptr<HistoricalDataBase>> historicalData;

    template <typename T>
    std::shared_ptr<T> MakesureHistoricalData(Period period)
    {
      auto it = historicalData.find(period);
      if (it != historicalData.end())
      {
        auto _data = std::dynamic_pointer_cast<T>(it->second);
        if (_data)
        {
          return _data;
        }
      }
      auto _data = std::make_shared<T>();
      historicalData[period] = _data;
      return _data;
    }

    void clearTimelinePoint()
    {
      auto timelineData = MakesureHistoricalData<TimelineData>(Period::TIMELINE);
      timelineData->Clear();
    }

    // 添加分时数据点
    void addTimelinePoint(const TimelinePoint &point)
    {
      auto timelineData = MakesureHistoricalData<TimelineData>(Period::TIMELINE);
      timelineData->data.push_back(point);
    }

    void addTimelinePoint(const CString &json_data);

    // 获取分时走势数据
    STOCK::TimelineData *getTimelineData()
    {
      return MakesureHistoricalData<TimelineData>(Period::TIMELINE).get();
    }
  };

  // 股票市场类，管理多个股票
  class StockMarket
  {
  private:
    std::map<std::wstring, std::shared_ptr<StockData>> stocks; // 以股票代码为键的股票数据映射

  public:
    void LoadRealtimeDataByJson(std::string data);
    void LoadTimelineDataByJson(std::wstring stock_id, CString *data);
    void LoadOKXDataByJson(const std::wstring& code, const std::string& json);  // OKX虚拟货币数据

    void ClearRealtimeData(bool includeOKX = true)
    {
      for (const auto &it : stocks)
      {
        // 如果不清除OKX数据，跳过okx_开头的代码
        if (!includeOKX && it.first.find(L"okx_") == 0)
        {
          continue;
        }
        RealTimeData data;
        it.second->realTimeData = data;
      }
    }

    // 添加股票
    std::shared_ptr<StockData> addStock(const std::wstring &code)
    {
      StockData stock;
      stock.info.code = code;
      stocks[code] = std::make_shared<StockData>(stock);
      return stocks[code];
    }

    // 获取股票数据
    std::shared_ptr<StockData> getStock(const std::wstring &code)
    {
      auto it = stocks.find(code);
      if (it != stocks.end())
      {
        return it->second;
      }
      return addStock(code);
    }

    // 更新股票最新数据
    void updateStockData(const std::wstring &code, const RealTimeData &data)
    {
      auto stock = getStock(code);
      if (stock)
      {
        stock->realTimeData = data;
      }
    }

    // 添加分时数据
    void addTimelinePoint(const std::wstring &code, const TimelinePoint &point)
    {
      auto stock = getStock(code);
      if (stock)
      {
        stock->addTimelinePoint(point);
      }
    }

    void addTimelinePoint(const std::wstring &code, const CString &json_data)
    {
      auto stock = getStock(code);
      if (stock)
      {
        stock->addTimelinePoint(json_data);
      }
    }
  };

} // namespace STOCK
