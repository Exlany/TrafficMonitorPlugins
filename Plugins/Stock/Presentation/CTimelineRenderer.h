#pragma once

#include <string>
#include <vector>
#include <Windows.h>
#include "../StockDef.h"

namespace StockPlugin {
namespace Presentation {

/// @brief 分时图渲染器
/// @details 负责分时图的绘制，从 FloatingWnd 中提取
class CTimelineRenderer
{
public:
    CTimelineRenderer();
    ~CTimelineRenderer();

    /// @brief 绘制完整的分时图
    /// @param hDC 设备上下文
    /// @param realtimeData 实时数据
    /// @param timelinePoints 分时数据点
    /// @param rect 绘制区域
    void Draw(HDC hDC, const STOCK::RealTimeData& realtimeData,
              const std::vector<STOCK::TimelinePoint>& timelinePoints,
              const RECT& rect);

    /// @brief 绘制加载中状态
    void DrawLoading(HDC hDC, const std::wstring& text, const RECT& rect);

    /// @brief 设置字体
    void SetFont(HFONT hFont);

private:
    HFONT m_hFont{nullptr};

    // 分区比例
    static constexpr float TIMELINE_HEIGHT_RATIO = 0.75f;
    static constexpr float GAP_HEIGHT_RATIO = 0.02f;

    // 时间偏移量
    static constexpr int BEFORE_NOON_OFFSET = 9 * 60 + 30;  // 9:30
    static constexpr int AFTER_NOON_OFFSET = 13 * 60 - 90;  // 午休90分钟
    static constexpr float TOTAL_TRADING_MINUTES = 240.0f;

    // 颜色常量
    static constexpr COLORREF TIMELINE_BG_COLOR = RGB(0, 0, 0);
    static constexpr COLORREF COLOR_GRID = RGB(50, 50, 50);
    static constexpr COLORREF COLOR_MIDDLE_LINE = RGB(100, 100, 100);
    static constexpr COLORREF COLOR_KLINE = RGB(255, 255, 255);
    static constexpr COLORREF COLOR_RISE = RGB(255, 80, 80);
    static constexpr COLORREF COLOR_FALL = RGB(80, 255, 80);
    static constexpr COLORREF COLOR_RISE_TEXT = RGB(255, 80, 80);
    static constexpr COLORREF COLOR_FALL_TEXT = RGB(80, 255, 80);
    static constexpr COLORREF COLOR_NEUTRAL = RGB(200, 200, 200);

    // 绘制子方法
    void DrawBackground(HDC hDC, const RECT& rect);
    void DrawGrid(HDC hDC, int w, int h, int timelineH);
    void DrawPriceLabels(HDC hDC, const STOCK::RealTimeData& realtimeData, int w, int timelineH);
    void DrawTimelineCurve(HDC hDC, const std::vector<STOCK::TimelinePoint>& timelinePoints,
                           const STOCK::RealTimeData& realtimeData, int x, int y, int w, int h);
    void DrawVolumeChart(HDC hDC, const std::vector<STOCK::TimelinePoint>& timelinePoints,
                         const std::vector<POINT>& dataPoints, STOCK::Price prevClosePrice,
                         int volumeTop, int volumeH, int w);

    // 坐标转换
    POINT Stock2Point(int x, int y, int w, int h, float unitY,
                      const STOCK::TimelinePoint& item, STOCK::Price prevClosePrice);
};

} // namespace Presentation
} // namespace StockPlugin
