#pragma once

#include <string>
#include <vector>
#include <map>
#include <shared_mutex>
#include "../Core/IConfigStore.h"

namespace StockPlugin {
namespace Domain {

/// @brief 多股票显示模式
enum class StockDisplayMode
{
    ShowAll = 0,    // 显示所有
    Carousel = 1,   // 轮播显示
    Manual = 2,     // 手动选择
    Smart = 3       // 智能模式（涨跌幅变化大的优先）
};

/// @brief 设置数据快照（不可变，线程安全）
/// @details 用于在不持有锁的情况下安全访问设置数据
struct SettingsSnapshot
{
    std::vector<std::wstring> stockCodes;
    std::map<std::wstring, std::wstring> aliases;
    bool fullDay{true};
    bool showStockName{true};
    bool colorWithPrice{true};
    int priceDecimal{3};
    unsigned int klineWidth{450};
    unsigned int klineHeight{210};
    StockDisplayMode displayMode{StockDisplayMode::ShowAll};
    int carouselInterval{5};
    bool checkUpdate{true};

    // 便捷方法
    std::wstring GetAlias(const std::wstring& code) const
    {
        auto it = aliases.find(code);
        return (it != aliases.end()) ? it->second : std::wstring();
    }
};

/// @brief 股票设置类
/// @details 封装所有配置数据，提供线程安全的访问
class CStockSettings
{
public:
    CStockSettings();
    ~CStockSettings();

    // 配置加载/保存
    void Load(Core::IConfigStore& store);
    void Save(Core::IConfigStore& store) const;

    /// @brief 创建设置快照（线程安全）
    /// @return 当前设置的不可变快照
    SettingsSnapshot CreateSnapshot() const;

    /// @brief 从快照更新设置（线程安全）
    /// @param snapshot 要应用的设置快照
    void ApplySnapshot(const SettingsSnapshot& snapshot);

    // 股票代码（线程安全）
    std::vector<std::wstring> GetStockCodes() const;
    void SetStockCodes(const std::vector<std::wstring>& codes);
    void AddStockCode(const std::wstring& code);
    void RemoveStockCode(const std::wstring& code);
    size_t GetStockCount() const;

    // 自定义别名
    std::map<std::wstring, std::wstring> GetAliases() const;
    void SetAliases(const std::map<std::wstring, std::wstring>& aliases);
    std::wstring GetAlias(const std::wstring& code) const;
    void SetAlias(const std::wstring& code, const std::wstring& alias);

    // 显示设置
    bool IsFullDay() const;
    void SetFullDay(bool value);

    bool ShowStockName() const;
    void SetShowStockName(bool value);

    bool ColorWithPrice() const;
    void SetColorWithPrice(bool value);

    int GetPriceDecimal() const;
    void SetPriceDecimal(int value);

    // K线图设置
    unsigned int GetKLineWidth() const;
    void SetKLineWidth(unsigned int value);

    unsigned int GetKLineHeight() const;
    void SetKLineHeight(unsigned int value);

    // 显示模式
    StockDisplayMode GetDisplayMode() const;
    void SetDisplayMode(StockDisplayMode mode);

    int GetCarouselInterval() const;
    void SetCarouselInterval(int seconds);

    // 更新设置
    bool IsCheckUpdateEnabled() const;
    void SetCheckUpdateEnabled(bool value);

private:
    mutable std::shared_mutex m_mutex;

    std::vector<std::wstring> m_stockCodes;
    std::map<std::wstring, std::wstring> m_aliases;
    bool m_fullDay{true};
    bool m_showStockName{true};
    bool m_colorWithPrice{true};
    int m_priceDecimal{3};
    unsigned int m_klineWidth{450};
    unsigned int m_klineHeight{210};
    StockDisplayMode m_displayMode{StockDisplayMode::ShowAll};
    int m_carouselInterval{5};
    bool m_checkUpdate{true};
};

} // namespace Domain
} // namespace StockPlugin
