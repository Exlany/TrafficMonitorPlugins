#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <Windows.h>

namespace STOCK {
    class StockData;
}

namespace StockPlugin {
namespace Presentation {

/// @brief 股票项渲染器
/// @details 负责股票信息的绘制，从 StockItem 中提取
class CStockItemRenderer
{
public:
    CStockItemRenderer();
    ~CStockItemRenderer();

    /// @brief 设置暗色模式
    void SetDarkMode(bool dark);

    /// @brief 设置是否显示股票名称
    void SetShowStockName(bool show);

    /// @brief 设置是否使用涨跌颜色
    void SetColorWithPrice(bool color);

    /// @brief 绘制单行股票信息
    /// @param hDC 设备上下文
    /// @param data 股票数据
    /// @param x, y, w, h 绘制区域
    void DrawSingleStock(HDC hDC, const STOCK::StockData& data, int x, int y, int w, int h);

    /// @brief 绘制单行股票信息（通过代码获取数据）
    void DrawStockRow(HDC hDC, const std::wstring& code, int x, int y, int w, int h,
                      std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> dataGetter);

    /// @brief 绘制多行股票（2xN 矩阵）
    void DrawMultiRow(HDC hDC, const std::vector<std::wstring>& codes, int x, int y, int w, int h,
                      bool showAll, size_t currentIndex, size_t secondIndex,
                      std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> dataGetter);

    /// @brief 计算单个股票的显示宽度
    int CalcStockDisplayWidth(HDC hDC, const std::wstring& code,
                              std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> dataGetter);

    /// @brief 计算多股票的总宽度
    int CalcTotalWidth(HDC hDC, const std::vector<std::wstring>& codes,
                       bool showAll, size_t currentIndex, size_t secondIndex,
                       std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> dataGetter);

private:
    bool m_darkMode{false};
    bool m_showStockName{true};
    bool m_colorWithPrice{true};

    COLORREF GetDefaultColor() const;
    COLORREF GetRiseColor() const;
    COLORREF GetFallColor() const;

    // 内部绘制方法
    void DrawStockRowInternal(HDC hDC, const STOCK::StockData& data, int x, int y, int w, int h);
};

} // namespace Presentation
} // namespace StockPlugin
