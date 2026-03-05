#pragma once

#include <string>
#include <vector>
#include <ctime>
#include <memory>
#include <functional>
#include "../Domain/CStockSettings.h"

namespace STOCK {
    class StockData;
}

namespace StockPlugin {
namespace Application {

/// @brief 显示管理器
/// @details 管理股票显示模式、轮播、手动切换等逻辑
class CDisplayManager
{
public:
    CDisplayManager();
    ~CDisplayManager();

    /// @brief 设置设置提供者
    void SetSettings(Domain::CStockSettings* settings);

    /// @brief 更新显示状态（每次 DataRequired 调用）
    /// @return 当前应显示的股票索引
    size_t Update();

    /// @brief 获取当前显示索引
    size_t GetCurrentIndex() const;

    /// @brief 获取第二行显示索引
    size_t GetSecondRowIndex() const;

    /// @brief 手动切换到下一只股票
    void SwitchToNext();

    /// @brief 获取智能模式下的最佳索引
    /// @param stockDataGetter 获取股票数据的回调
    int GetSmartIndex(std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> stockDataGetter) const;

    /// @brief 重置显示状态
    void Reset();

private:
    Domain::CStockSettings* m_settings{nullptr};
    size_t m_currentIndex{0};
    time_t m_lastCarouselTime{0};
};

} // namespace Application
} // namespace StockPlugin
