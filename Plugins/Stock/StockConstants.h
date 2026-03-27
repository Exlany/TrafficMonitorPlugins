#pragma once

// ============================================================================
// Stock 插件常量定义
// ============================================================================

namespace StockConstants
{
    // ------------------------------------------------------------------------
    // 产品类型枚举
    // ------------------------------------------------------------------------
    enum class ProductType
    {
        Unknown = 0,
        SH_Stock,       // 上证股票
        SZ_Stock,       // 深证股票
        BJ_Stock,       // 北交所股票
        CN_Index,       // A股指数
        ETF,            // ETF基金
        HK_Stock,       // 港股
        US_Stock,       // 美股个股
        US_Index,       // 美股指数
        Crypto_OKX,     // OKX虚拟货币
        Crypto_Binance, // 币安虚拟货币
        Futures,        // 期货
        Forex           // 外汇
    };

    // ------------------------------------------------------------------------
    // 市场前缀
    // ------------------------------------------------------------------------
    constexpr auto kSH = L"sh";       // 上海证券交易所
    constexpr auto kSZ = L"sz";       // 深圳证券交易所
    constexpr auto kBJ = L"bj";       // 北京证券交易所
    constexpr auto kHK = L"rt_hk";    // 香港交易所
    constexpr auto kMG = L"gb_";      // 美股个股
    constexpr auto kMGI = L"int_";    // 美股指数/国际指数
    constexpr auto kOKX = L"okx_";    // OKX虚拟货币
    constexpr auto kHF = L"hf_";      // 期货(黄金、原油等)
    constexpr auto kBN = L"bn_";      // 币安虚拟货币
    constexpr auto kFX = L"fx_";      // 外汇

    // ------------------------------------------------------------------------
    // 数据长度常量 (新浪API返回的字段数)
    // ------------------------------------------------------------------------
    constexpr size_t DATA_LEN_MG = 36;   // 美股
    constexpr size_t DATA_LEN_SH = 34;   // 上证A股 & 指数
    constexpr size_t DATA_LEN_SZ = 33;   // 深证A股 & 指数
    constexpr size_t DATA_LEN_BJ = 39;   // 北证A股 & 指数
    constexpr size_t DATA_LEN_HK = 25;   // 港股
    constexpr size_t DATA_LEN_INT = 4;   // 国际指数/期货

    // ------------------------------------------------------------------------
    // 交易时间常量
    // ------------------------------------------------------------------------
    constexpr int TRADING_START_HOUR = 9;      // 交易开始时间(小时)
    constexpr int TRADING_START_MINUTE = 30;   // 交易开始时间(分钟)
    constexpr int TRADING_END_HOUR = 15;       // 交易结束时间(小时)
    constexpr int TRADING_END_MINUTE = 0;      // 交易结束时间(分钟)
    constexpr int LUNCH_BREAK_START = 11;      // 午休开始(小时)
    constexpr int LUNCH_BREAK_END = 13;        // 午休结束(小时)

    // ------------------------------------------------------------------------
    // 请求间隔常量
    // ------------------------------------------------------------------------
    constexpr int REQUEST_INTERVAL_SEC = 3;        // 实时数据请求间隔(秒)
    constexpr int TIMELINE_REQUEST_INTERVAL = 10;  // 分时数据请求间隔(秒)
    constexpr int NETWORK_CHECK_INTERVAL = 5000;   // 网络检查间隔(毫秒)
    constexpr int NETWORK_CHECK_MAX_RETRIES = 12;  // 网络检查最大重试次数

    // ------------------------------------------------------------------------
    // 显示限制常量
    // ------------------------------------------------------------------------
    constexpr int MAX_STOCK_ITEMS = 10;            // 最大股票显示数量
    constexpr int DEFAULT_KLINE_WIDTH = 450;       // 默认K线图宽度
    constexpr int DEFAULT_KLINE_HEIGHT = 210;      // 默认K线图高度
    constexpr int DEFAULT_CAROUSEL_INTERVAL = 5;   // 默认轮播间隔(秒)
    constexpr int DEFAULT_PRICE_DECIMAL = 3;       // 默认价格小数位数
    constexpr int DEFAULT_TOOLTIP_MAX_ITEMS = 6;   // tooltip 默认最多展示条目
    constexpr int MIN_TOOLTIP_MAX_ITEMS = 1;       // tooltip 最小展示条目
    constexpr int MAX_TOOLTIP_MAX_ITEMS = 20;      // tooltip 最大展示条目
    constexpr int DEFAULT_ALERT_CHANGE_PERCENT = 5; // 默认涨跌幅预警阈值(%)
    constexpr int MIN_ALERT_CHANGE_PERCENT = 1;    // 预警阈值最小值(%)
    constexpr int MAX_ALERT_CHANGE_PERCENT = 30;   // 预警阈值最大值(%)

    // ------------------------------------------------------------------------
    // 分时图时间计算常量
    // ------------------------------------------------------------------------
    constexpr int BEFORE_NOON_OFFSET = 570;    // 9:30 = 9.5 * 60
    constexpr int AFTER_NOON_OFFSET = 660;     // 9:30 + 1.5h午休 = 11 * 60
    constexpr float TOTAL_TRADING_MINUTES = 240.0f;  // 4小时 = 240分钟

    // ------------------------------------------------------------------------
    // 分时图布局比例
    // ------------------------------------------------------------------------
    constexpr float TIMELINE_HEIGHT_RATIO = 0.68f;   // 分时图高度占比
    constexpr float GAP_HEIGHT_RATIO = 0.04f;        // 间隔高度占比
    constexpr float VOLUME_HEIGHT_RATIO = 0.28f;     // 成交量高度占比

    // ------------------------------------------------------------------------
    // 智能模式权重
    // ------------------------------------------------------------------------
    constexpr double DAILY_CHANGE_WEIGHT = 0.4;      // 当日涨跌幅权重
    constexpr double RECENT_CHANGE_WEIGHT = 0.6;     // 近期涨跌幅权重
    constexpr int RECENT_MINUTES = 10;               // 近期时间窗口(分钟)

    // ------------------------------------------------------------------------
    // 颜色常量 (RGB)
    // 注意：避免使用 COLOR_BACKGROUND 等 Windows 已定义的宏名
    // ------------------------------------------------------------------------
    constexpr COLORREF COLOR_RISE = RGB(195, 0, 0);        // 上涨颜色(红)
    constexpr COLORREF COLOR_FALL = RGB(46, 139, 87);      // 下跌颜色(绿)
    constexpr COLORREF COLOR_RISE_TEXT = RGB(179, 64, 65); // 上涨文字颜色
    constexpr COLORREF COLOR_FALL_TEXT = RGB(44, 144, 51); // 下跌文字颜色
    constexpr COLORREF COLOR_NEUTRAL = RGB(154, 151, 157); // 中性颜色
    constexpr COLORREF COLOR_GRID = RGB(240, 240, 240);    // 网格线颜色
    constexpr COLORREF COLOR_MIDDLE_LINE = RGB(140, 140, 140); // 中线颜色
    constexpr COLORREF COLOR_KLINE = RGB(70, 113, 152);    // K线颜色
    constexpr COLORREF KLINE_BACKGROUND = RGB(255, 255, 255); // 背景颜色

    // ------------------------------------------------------------------------
    // HTTP超时常量
    // ------------------------------------------------------------------------
    constexpr int HTTP_CONNECT_TIMEOUT = 10000;    // 连接超时(毫秒)
    constexpr int HTTP_RECEIVE_TIMEOUT = 10000;    // 接收超时(毫秒)
    constexpr int HTTP_DOWNLOAD_TIMEOUT = 30000;   // 下载超时(毫秒)

} // namespace StockConstants
