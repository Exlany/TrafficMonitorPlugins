#include "pch.h"
#include "CTimelineRenderer.h"
#include "../Common.h"
#include <algorithm>

#undef min
#undef max

namespace StockPlugin {
namespace Presentation {

CTimelineRenderer::CTimelineRenderer() = default;

CTimelineRenderer::~CTimelineRenderer() = default;

void CTimelineRenderer::SetFont(HFONT hFont)
{
    m_hFont = hFont;
}

void CTimelineRenderer::Draw(HDC hDC, const STOCK::RealTimeData& realtimeData,
                              const std::vector<STOCK::TimelinePoint>& timelinePoints,
                              const RECT& rect)
{
    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return;

    int w = rect.right - rect.left;
    int totalH = rect.bottom - rect.top;
    int x = rect.left;
    int y = rect.top;

    // 分区计算
    int timelineH = static_cast<int>(totalH * TIMELINE_HEIGHT_RATIO);
    int h = timelineH;

    // 绘制网格
    DrawGrid(hDC, w, h, timelineH);

    if (!timelinePoints.empty())
    {
        DrawPriceLabels(hDC, realtimeData, w, timelineH);
        DrawTimelineCurve(hDC, timelinePoints, realtimeData, x, y, w, h);
    }
}

void CTimelineRenderer::DrawLoading(HDC hDC, const std::wstring& text, const RECT& rect)
{
    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return;

    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;

    pDC->SetTextColor(COLOR_NEUTRAL);
    pDC->SetBkMode(TRANSPARENT);

    CString loadingText(text.c_str());
    int textWidth = pDC->GetTextExtent(loadingText).cx;
    pDC->TextOut((w - textWidth) / 2, 10, loadingText);
}

void CTimelineRenderer::DrawBackground(HDC hDC, const RECT& rect)
{
    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return;

    CRect r(rect);
    pDC->FillSolidRect(r, TIMELINE_BG_COLOR);
    pDC->SetBkMode(TRANSPARENT);
}

void CTimelineRenderer::DrawGrid(HDC hDC, int w, int h, int timelineH)
{
    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return;

    CPen pGrid(PS_DOT, 1, COLOR_GRID);
    CPen* pOldPen = pDC->SelectObject(&pGrid);

    // 分时图网格线
    pDC->MoveTo(0, h / 4);
    pDC->LineTo(w, h / 4);
    pDC->MoveTo(0, h / 4 * 3);
    pDC->LineTo(w, h / 4 * 3);

    pDC->MoveTo(w / 4, 0);
    pDC->LineTo(w / 4, h);
    pDC->MoveTo(w / 2, 0);
    pDC->LineTo(w / 2, h);
    pDC->MoveTo(w / 4 * 3, 0);
    pDC->LineTo(w / 4 * 3, h);

    CPen pMiddleLine(PS_DASHDOT, 1, COLOR_MIDDLE_LINE);
    pDC->SelectObject(&pMiddleLine);
    pDC->MoveTo(0, h / 2);
    pDC->LineTo(w, h / 2);

    // 成交量区域分隔线
    pDC->MoveTo(0, timelineH);
    pDC->LineTo(w, timelineH);

    pDC->SelectObject(pOldPen);
}

void CTimelineRenderer::DrawPriceLabels(HDC hDC, const STOCK::RealTimeData& realtimeData, int w, int timelineH)
{
    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return;

    STOCK::Price priceLimit = realtimeData.priceLimit;
    CRect timelineRect(0, 0, w, timelineH);

    // 涨停价
    pDC->SetTextColor(COLOR_RISE_TEXT);
    float upperLimitPrice = static_cast<float>(realtimeData.prevClosePrice + priceLimit);
    CString upperLimitTxt;
    upperLimitTxt.Format(_T("%.2f"), upperLimitPrice);
    CRect upperLimitTxtRect{timelineRect};
    upperLimitTxtRect.right = upperLimitTxtRect.left + pDC->GetTextExtent(upperLimitTxt).cx;
    pDC->DrawText(upperLimitTxt, upperLimitTxtRect, DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

    CString upperLimitRateTxt;
    upperLimitRateTxt.Format(_T("%.2f%%"), priceLimit * 100.0 / realtimeData.prevClosePrice);
    CRect upperLimitRateTxtRect{timelineRect};
    upperLimitRateTxtRect.left = w - (upperLimitRateTxtRect.left + pDC->GetTextExtent(upperLimitRateTxt).cx);
    pDC->DrawText(upperLimitRateTxt, upperLimitRateTxtRect, DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

    // 跌停价
    pDC->SetTextColor(COLOR_FALL_TEXT);
    float lowerLimitPrice = static_cast<float>(realtimeData.prevClosePrice - priceLimit);
    CString lowerLimitTxt;
    lowerLimitTxt.Format(_T("%.2f"), lowerLimitPrice);
    CRect lowerLimitTxtRect{timelineRect};
    lowerLimitTxtRect.right = lowerLimitTxtRect.left + pDC->GetTextExtent(lowerLimitTxt).cx;
    pDC->DrawText(lowerLimitTxt, lowerLimitTxtRect, DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);

    CString lowerLimitRateTxt;
    lowerLimitRateTxt.Format(_T("-%.2f%%"), priceLimit * 100.0 / realtimeData.prevClosePrice);
    CRect lowerLimitRateTxtRect{timelineRect};
    lowerLimitRateTxtRect.left = w - (lowerLimitRateTxtRect.left + pDC->GetTextExtent(lowerLimitRateTxt).cx);
    pDC->DrawText(lowerLimitRateTxt, lowerLimitRateTxtRect, DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);

    // 昨收价
    pDC->SetTextColor(COLOR_NEUTRAL);
    CString middleTxt;
    middleTxt.Format(_T("%.2f"), realtimeData.prevClosePrice);
    CRect middleTxtRect{timelineRect};
    middleTxtRect.right = middleTxtRect.left + pDC->GetTextExtent(middleTxt).cx;
    pDC->DrawText(middleTxt, middleTxtRect, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

POINT CTimelineRenderer::Stock2Point(int x, int y, int w, int h, float unitY,
                                      const STOCK::TimelinePoint& item, STOCK::Price prevClosePrice)
{
    POINT p = {0, 0};
    std::vector<std::string> time_arr = CCommon::split(item.time, ":");
    if (time_arr.size() >= 2)
    {
        int hour = atoi(time_arr[0].c_str());
        int minute = atoi(time_arr[1].c_str());

        int countX = hour * 60 + minute;

        if (hour < 12)
        {
            countX -= BEFORE_NOON_OFFSET;
        }
        else if (hour >= 13)
        {
            countX -= AFTER_NOON_OFFSET;
        }
        p.x = static_cast<int>(w / TOTAL_TRADING_MINUTES * countX);
    }
    p.y = static_cast<int>((item.price - prevClosePrice) * unitY * 100);
    return p;
}

void CTimelineRenderer::DrawTimelineCurve(HDC hDC, const std::vector<STOCK::TimelinePoint>& timelinePoints,
                                           const STOCK::RealTimeData& realtimeData, int x, int y, int w, int h)
{
    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return;

    float halfH = h / 2.0f;
    STOCK::Price priceLimit = realtimeData.priceLimit;
    float unitY = priceLimit != 0 ? halfH / static_cast<float>(priceLimit * 100) : 0;

    CPen pKLine(PS_SOLID, 1, COLOR_KLINE);
    CPen* pOldPen = pDC->SelectObject(&pKLine);

    std::vector<POINT> dataPoints;
    for (const STOCK::TimelinePoint& item : timelinePoints)
    {
        POINT p = Stock2Point(x, y, w, h, unitY, item, realtimeData.prevClosePrice);
        dataPoints.push_back(p);
    }

    int startY = static_cast<int>(halfH - (realtimeData.openPrice - realtimeData.prevClosePrice) * unitY * 100);
    pDC->MoveTo(x, startY);
    for (size_t i = 0; i < dataPoints.size(); i++)
    {
        int pX = dataPoints[i].x;
        int pY = static_cast<int>(halfH - dataPoints[i].y);
        pDC->LineTo(pX, pY);
    }

    pDC->SelectObject(pOldPen);

    // 绘制成交量柱状图
    int totalH = static_cast<int>(h / TIMELINE_HEIGHT_RATIO);
    int gap = static_cast<int>(totalH * GAP_HEIGHT_RATIO);
    int volumeH = totalH - h - gap;
    int volumeTop = h + gap;
    DrawVolumeChart(hDC, timelinePoints, dataPoints, realtimeData.prevClosePrice, volumeTop, volumeH, w);
}

void CTimelineRenderer::DrawVolumeChart(HDC hDC, const std::vector<STOCK::TimelinePoint>& timelinePoints,
                                         const std::vector<POINT>& dataPoints, STOCK::Price prevClosePrice,
                                         int volumeTop, int volumeH, int w)
{
    if (timelinePoints.empty() || dataPoints.empty())
        return;

    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return;

    // 计算最大成交量用于Y轴缩放
    STOCK::Volume maxVolume = 0;
    for (const auto& point : timelinePoints)
    {
        if (point.volume > maxVolume)
            maxVolume = point.volume;
    }

    if (maxVolume == 0)
        return;

    // 计算柱状图宽度
    int barWidth = std::max(1, w / static_cast<int>(timelinePoints.size()) - 1);
    if (barWidth < 1)
        barWidth = 1;

    // 绘制每个成交量柱
    for (size_t i = 0; i < timelinePoints.size() && i < dataPoints.size(); i++)
    {
        const auto& point = timelinePoints[i];
        int barHeight = static_cast<int>((static_cast<double>(point.volume) / maxVolume) * volumeH);
        if (barHeight < 1)
            barHeight = 1;

        int barX = dataPoints[i].x;
        int barY = volumeTop + volumeH - barHeight;

        // 判断涨跌颜色
        COLORREF barColor;
        if (i == 0)
        {
            barColor = (point.price >= prevClosePrice) ? COLOR_RISE : COLOR_FALL;
        }
        else
        {
            barColor = (point.price >= timelinePoints[i - 1].price) ? COLOR_RISE : COLOR_FALL;
        }

        // 绘制柱状
        CBrush brush(barColor);
        CBrush* pOldBrush = pDC->SelectObject(&brush);
        CPen pen(PS_SOLID, 1, barColor);
        CPen* pOldPen = pDC->SelectObject(&pen);

        pDC->Rectangle(barX - barWidth / 2, barY, barX + barWidth / 2 + 1, volumeTop + volumeH);

        pDC->SelectObject(pOldBrush);
        pDC->SelectObject(pOldPen);
    }
}

} // namespace Presentation
} // namespace StockPlugin
