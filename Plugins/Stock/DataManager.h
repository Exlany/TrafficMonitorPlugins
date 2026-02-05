#pragma once
#include <string>
#include <map>
#include "resource.h"
#include "StockDef.h"
#include <mutex>

using namespace STOCK;

#define g_data CDataManager::Instance()

// 多股票显示模式
enum class StockDisplayMode
{
    ShowAll = 0,    // 显示所有
    Carousel = 1,   // 轮播显示
    Manual = 2,     // 手动选择
    Smart = 3       // 智能模式（涨跌幅变化大的优先）
};

struct SettingData
{
    vector<std::wstring> m_stock_codes;                      // 代码
    std::map<std::wstring, std::wstring> m_stock_aliases;    // 自定义别名 (代码 -> 别名)
    bool m_full_day;                                       // 全天更新
    bool m_show_stock_name{};                                // 显示股票名称
    bool m_color_with_price{};                               // 涨跌颜色标识
    unsigned m_kline_width;                                  // 走势图宽度
    unsigned m_kline_height;                                 // 走势图高度
    int m_price_decimal{3};                                  // 价格小数位数（2或3）
    StockDisplayMode m_display_mode{StockDisplayMode::ShowAll}; // 显示模式
    int m_carousel_interval{5};                              // 轮播间隔（秒）
    bool m_check_update{true};                               // 是否检查更新
};

// Stock显示数据
// struct StockInfo
// {
//     std::wstring pc = L"--%";
//     std::wstring p = L"--";
//     std::wstring name = L"";
//     std::wstring ToString(bool include_name = true) const;
//     bool IsEmpty() const;
// };

class CDataManager
{
private:
    CDataManager();
    ~CDataManager();

public:
    static CDataManager &Instance();

    void LoadConfig(const std::wstring &config_dir);
    void SaveConfig();
    const CString &StringRes(UINT id); // 根据资源id获取一个字符串资源
    void DPIFromWindow(CWnd *pWnd);
    int DPI(int pixel);
    float DPIF(float pixel);
    int RDPI(int pixel);
    HICON GetIcon(UINT id);
    void ResetText();
    std::shared_ptr<StockData> GetStockData(const std::wstring &code);

    // 获取最新数据
    void RequestRealtimeData();
    void RequestTimelineData(std::wstring stock_id);
    void RequestOKXData(const std::wstring& code);  // OKX虚拟货币数据

    SettingData m_setting_data;
    std::wstring m_log_path;
    bool m_right_align{}; // 数值是否右对齐
    UINT m_system_code_page{}; // 系统默认代码页（936=GBK, 65001=UTF-8）

private:
    static CDataManager m_instance;

    std::wstring m_config_path;

    std::map<UINT, CString> m_string_table;
    std::map<UINT, HICON> m_icons;
    int m_dpi{96};

    STOCK::StockMarket stockMarket;
};
