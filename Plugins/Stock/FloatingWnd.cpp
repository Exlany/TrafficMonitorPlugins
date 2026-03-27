#include "pch.h"
#include "FloatingWnd.h"
#include "StockConstants.h"
#include <afxinet.h>
#include <memory>
#include <algorithm>
#include <cmath>
#include "Common.h"
#include "DataManager.h"

// 使用显式命名空间限定代替 using namespace

BEGIN_MESSAGE_MAP(CFloatingWnd, CWnd)
ON_WM_PAINT()
ON_WM_ERASEBKGND()
ON_WM_LBUTTONDOWN()
ON_WM_ACTIVATE()
ON_WM_CREATE()
ON_MESSAGE(FWND_MSG_UPDATE_STATUS, OnUpdateStatus)
ON_MESSAGE(FWND_MSG_REQUEST_DATA, OnRequestData)
END_MESSAGE_MAP()

int CFloatingWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    PostMessage(FWND_MSG_REQUEST_DATA, time(nullptr), 0);
    return 0;
}

// 处理消息
LRESULT CFloatingWnd::OnUpdateStatus(WPARAM wParam, LPARAM lParam)
{
    Invalidate();
    return 0;
}

LRESULT CFloatingWnd::OnRequestData(WPARAM wParam, LPARAM lParam)
{
    time_t req_time = (time_t)wParam;
    if (req_time)
    {
        unsigned __int64 last_req = m_last_request_time.load();
        if (req_time - static_cast<time_t>(last_req) > StockConstants::TIMELINE_REQUEST_INTERVAL)
        {
            m_last_request_time.store(static_cast<unsigned __int64>(req_time));
            // 开始网络请求
            RequestData();
            // 重置加载文本
            {
                std::lock_guard<std::mutex> lock(m_loadingTextMutex);
                m_loadingStateText = g_data.StringRes(IDS_LOADING).GetString();
            }
        }
        else
        {
            // 更新加载动画（限制最大长度）
            std::lock_guard<std::mutex> lock(m_loadingTextMutex);
            if (m_loadingStateText.GetLength() < 50)
            {
                m_loadingStateText += L".";
            }
        }
        Invalidate();
    }
    return 0;
}

CFloatingWnd::CFloatingWnd()
{
}

CFloatingWnd::~CFloatingWnd()
{
    // 标记窗口正在销毁
    m_isDestroying.store(true);

    // 等待线程完成（最多等待 2 秒）
    int waitCount = 0;
    while (m_isThreadRunning.load() && waitCount < 20)
    {
        Sleep(100);
        waitCount++;
    }

    if (m_transparentWnd.GetSafeHwnd())
        m_transparentWnd.DestroyWindow();
}

BOOL CFloatingWnd::Create(CFont *font, CPoint pt, std::wstring stock_id)
{
    {
        std::lock_guard<std::mutex> lock(m_stockIdMutex);
        m_stockId = stock_id;
    }
    // 注册窗口类
    WNDCLASS wndcls;
    HINSTANCE hInst = AfxGetInstanceHandle();
    if (!(::GetClassInfo(hInst, L"CTransparentWnd", &wndcls)))
    {
        wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndcls.lpfnWndProc = ::DefWindowProc;
        wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
        wndcls.hInstance = hInst;
        wndcls.hIcon = NULL;
        wndcls.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndcls.hbrBackground = NULL;
        wndcls.lpszMenuName = NULL;
        wndcls.lpszClassName = L"CTransparentWnd";
        if (!AfxRegisterClass(&wndcls))
            return FALSE;
    }

    // 设置父窗口指针
    m_transparentWnd.SetParent(this);

    m_pFont = font;

    // 获取包含鼠标点的显示器
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {sizeof(MONITORINFO)};
    GetMonitorInfo(hMonitor, &mi);
    CRect screenRect = mi.rcWork; // 工作区域

    // 创建透明全屏窗口
    if (!m_transparentWnd.CreateEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                                    L"CTransparentWnd", L"", WS_POPUP | WS_VISIBLE,
                                    screenRect, NULL, 0, NULL))
    {
        TRACE(L"Failed to create transparent window\n");
        return FALSE;
    }

    const SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const int WIDTH = g_data.RDPI(settings.klineWidth);
    const int HEIGHT = g_data.RDPI(settings.klineHeight);
    const int popupOffsetX = g_data.RDPI(16);
    const int popupOffsetY = g_data.RDPI(18);
    int x = pt.x + popupOffsetX;
    int y = pt.y + popupOffsetY;

    // 调整位置
    if (x + WIDTH > screenRect.right)
        x = pt.x - WIDTH - popupOffsetX;
    if (y + HEIGHT > screenRect.bottom)
        y = pt.y - HEIGHT - popupOffsetY;
    x = (std::max)(static_cast<int>(screenRect.left), x);
    y = (std::max)(static_cast<int>(screenRect.top), y);

    CRect rect(x, y, x + WIDTH, y + HEIGHT);

    // 创建实际的浮动窗口
    if (!CreateEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                  AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW),
                  L"", WS_POPUP | WS_VISIBLE | WS_BORDER,
                  rect, &m_transparentWnd, 0))
    {
        TRACE(L"Failed to create floating window\n");
        m_transparentWnd.DestroyWindow();
        return FALSE;
    }

    // 确保浮动窗口在最顶层
    BringWindowToTop();
    SetForegroundWindow();

    // 设置近乎不可见的遮罩层，用于处理浮窗之外的点击关闭
    m_transparentWnd.SetLayeredWindowAttributes(0, 1, LWA_ALPHA);
    m_transparentWnd.ShowWindow(SW_SHOW);

    TRACE(L"Windows created successfully\n");
    return TRUE;
}

CPoint CFloatingWnd::Stock2Point(int x, int y, int w, int h, float unitY, const STOCK::TimelinePoint &item, const STOCK::Price prevClosePrice)
{
    CPoint p = CPoint();
    std::vector<std::string> time_arr = CCommon::split(item.time, ":");
    if (time_arr.size() >= 2)
    {
        int hour = _ttoi(CString(time_arr[0].c_str()));
        int minute = _ttoi(CString(time_arr[1].c_str()));

        int countX = hour * 60 + minute;

        if (hour < 12)
        {
            countX -= StockConstants::BEFORE_NOON_OFFSET;
        }
        else if (hour >= 13)
        {
            countX -= StockConstants::AFTER_NOON_OFFSET;
        }
        p.x = static_cast<int>(w / StockConstants::TOTAL_TRADING_MINUTES * countX);
    }
    p.y = static_cast<int>((item.price - prevClosePrice) * unitY * 100);
    return p;
}

void CFloatingWnd::DrawGrid(CDC *pDC, int w, int h, int timelineH)
{
    CPen pGrid(PS_DOT, 1, StockConstants::COLOR_GRID);
    CPen *pOldPen = pDC->SelectObject(&pGrid);

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

    CPen pMiddleLine(PS_DASHDOT, 1, StockConstants::COLOR_MIDDLE_LINE);
    pDC->SelectObject(&pMiddleLine);
    pDC->MoveTo(0, h / 2);
    pDC->LineTo(w, h / 2);

    // 成交量区域分隔线
    pDC->MoveTo(0, timelineH);
    pDC->LineTo(w, timelineH);

    pDC->SelectObject(pOldPen);
}

void CFloatingWnd::DrawPriceLabels(CDC *pDC, const STOCK::RealTimeData &realtimeData, int w, int timelineH, STOCK::Price displayPriceLimit)
{
    STOCK::Price priceLimit = displayPriceLimit;
    CRect timelineRect(0, 0, w, timelineH);

    pDC->SetTextColor(StockConstants::COLOR_RISE_TEXT);
    float upperLimitPrice = static_cast<float>(realtimeData.prevClosePrice + priceLimit);
    CString upperLimitTxt;
    upperLimitTxt.Format(_T("%.2f"), upperLimitPrice);
    CRect upperLimitTxtRect{timelineRect};
    upperLimitTxtRect.right = upperLimitTxtRect.left + pDC->GetTextExtent(upperLimitTxt).cx;
    pDC->DrawText(upperLimitTxt, upperLimitTxtRect, DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

    CString upperLimitRateTxt;
    const double safePrevClose = (realtimeData.prevClosePrice > 0.0) ? realtimeData.prevClosePrice : 1.0;
    upperLimitRateTxt.Format(_T("%.2f%%"), priceLimit * 100.0 / safePrevClose);
    CRect upperLimitRateTxtRect{timelineRect};
    upperLimitRateTxtRect.left = w - (upperLimitRateTxtRect.left + pDC->GetTextExtent(upperLimitRateTxt).cx);
    pDC->DrawText(upperLimitRateTxt, upperLimitRateTxtRect, DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

    pDC->SetTextColor(StockConstants::COLOR_FALL_TEXT);
    float lowerLimitPrice = static_cast<float>(realtimeData.prevClosePrice - priceLimit);
    CString lowerLimitTxt;
    lowerLimitTxt.Format(_T("%.2f"), lowerLimitPrice);
    CRect lowerLimitTxtRect{timelineRect};
    lowerLimitTxtRect.right = lowerLimitTxtRect.left + pDC->GetTextExtent(lowerLimitTxt).cx;
    pDC->DrawText(lowerLimitTxt, lowerLimitTxtRect, DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);

    CString lowerLimitRateTxt;
    lowerLimitRateTxt.Format(_T("-%.2f%%"), priceLimit * 100.0 / safePrevClose);
    CRect lowerLimitRateTxtRect{timelineRect};
    lowerLimitRateTxtRect.left = w - (lowerLimitRateTxtRect.left + pDC->GetTextExtent(lowerLimitRateTxt).cx);
    pDC->DrawText(lowerLimitRateTxt, lowerLimitRateTxtRect, DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);

    pDC->SetTextColor(StockConstants::COLOR_NEUTRAL);
    CString middleTxt;
    middleTxt.Format(_T("%.2f"), realtimeData.prevClosePrice);
    CRect middleTxtRect{timelineRect};
    middleTxtRect.right = middleTxtRect.left + pDC->GetTextExtent(middleTxt).cx;
    pDC->DrawText(middleTxt, middleTxtRect, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void CFloatingWnd::DrawTimelineCurve(CDC *pDC, const std::vector<STOCK::TimelinePoint> &timelinePoint,
                                      const STOCK::RealTimeData &realtimeData, int x, int y, int w, int h, STOCK::Price displayPriceLimit)
{
    float halfH = h / 2.0f;
    STOCK::Price priceLimit = displayPriceLimit;
    float unitY = priceLimit != 0 ? halfH / static_cast<float>(priceLimit * 100) : 0;

    CPen pKLine(PS_SOLID, 1, StockConstants::COLOR_KLINE);
    CPen *pOldPen = pDC->SelectObject(&pKLine);

    std::vector<CPoint> dataPoints;
    for (const STOCK::TimelinePoint &item : timelinePoint)
    {
        CPoint p = Stock2Point(x, y, w, h, unitY, item, realtimeData.prevClosePrice);
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
    int totalH = static_cast<int>(h / StockConstants::TIMELINE_HEIGHT_RATIO);
    int gap = static_cast<int>(totalH * StockConstants::GAP_HEIGHT_RATIO);
    int volumeH = totalH - h - gap;
    int volumeTop = h + gap;
    DrawVolumeChart(pDC, timelinePoint, dataPoints, realtimeData.prevClosePrice, volumeTop, volumeH, w);
}

void CFloatingWnd::DrawSummaryHeader(CDC* pDC, const CRect& headerRect, const std::wstring& stockName,
                                     const std::wstring& stockCode, const STOCK::RealTimeData& realtimeData,
                                     const std::string& lastTimelineTime, bool hasRealtimeData, bool hasTimelineData)
{
    CRect bgRect(headerRect);
    pDC->FillSolidRect(bgRect, RGB(247, 249, 252));

    CPen dividerPen(PS_SOLID, 1, RGB(228, 232, 238));
    CPen* oldPen = pDC->SelectObject(&dividerPen);
    pDC->MoveTo(headerRect.left, headerRect.bottom - 1);
    pDC->LineTo(headerRect.right, headerRect.bottom - 1);
    pDC->SelectObject(oldPen);

    const int padding = g_data.RDPI(8);
    const int lineHeight = g_data.RDPI(16);
    const CRect closeButtonRect = GetCloseButtonRect(headerRect);

    CString title;
    if (!stockName.empty() && !stockCode.empty())
    {
        title.Format(L"%s (%s)", stockName.c_str(), stockCode.c_str());
    }
    else if (!stockName.empty())
    {
        title = stockName.c_str();
    }
    else if (!stockCode.empty())
    {
        title = stockCode.c_str();
    }
    else
    {
        title = L"--";
    }

    CString valueText;
    if (hasRealtimeData)
    {
        valueText.Format(L"%s  %s", realtimeData.displayPrice.c_str(), realtimeData.displayFluctuation.c_str());
    }
    else
    {
        valueText = L"--  --%";
    }

    CRect titleRect(headerRect.left + padding, headerRect.top + g_data.RDPI(4),
                    closeButtonRect.left - g_data.RDPI(6), headerRect.top + g_data.RDPI(4) + lineHeight);
    CRect valueRect(headerRect.left + padding, headerRect.top + g_data.RDPI(4),
                    closeButtonRect.left - g_data.RDPI(6), headerRect.top + g_data.RDPI(4) + lineHeight);

    const int valueWidth = pDC->GetTextExtent(valueText).cx;
    titleRect.right -= valueWidth + g_data.RDPI(10);
    valueRect.left = (std::max)(valueRect.left, valueRect.right - valueWidth);

    pDC->SetTextColor(RGB(35, 45, 55));
    pDC->DrawText(title, titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    pDC->SetTextColor(GetFluctuationColor(realtimeData, hasRealtimeData));
    pDC->DrawText(valueText, valueRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    CString statusLine;
    CString updateTime = hasTimelineData
                             ? CString(CCommon::StrToUnicode(lastTimelineTime.c_str()).c_str())
                             : CString(L"--:--");
    statusLine.Format(g_data.StringRes(IDS_LAST_UPDATE_TIME), updateTime.GetString());

    CRect statusRect(headerRect.left + padding, headerRect.top + g_data.RDPI(4) + lineHeight,
                     closeButtonRect.left - g_data.RDPI(6), headerRect.bottom - g_data.RDPI(2));
    pDC->SetTextColor(StockConstants::COLOR_NEUTRAL);
    pDC->DrawText(statusLine, statusRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    DrawCloseButton(pDC, headerRect);
}

void CFloatingWnd::DrawCenteredStatusText(CDC* pDC, const CRect& drawRect, const CString& statusText, COLORREF textColor)
{
    pDC->SetTextColor(textColor);
    CRect textRect(drawRect);
    pDC->DrawText(statusText, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

STOCK::Price CFloatingWnd::CalculateDisplayPriceLimit(const STOCK::RealTimeData& realtimeData,
                                                      const std::vector<STOCK::TimelinePoint>& timelinePoint) const
{
    STOCK::Price priceLimit = realtimeData.priceLimit;
    if (priceLimit > 0.0)
        return priceLimit;

    const STOCK::Price prevClose = realtimeData.prevClosePrice;
    if (prevClose <= 0.0)
        return 0.01;

    STOCK::Price maxDiff = 0.0;
    for (const auto& point : timelinePoint)
    {
        maxDiff = (std::max)(maxDiff, std::abs(point.price - prevClose));
    }
    maxDiff = (std::max)(maxDiff, std::abs(realtimeData.currentPrice - prevClose));

    if (maxDiff <= 0.0)
        maxDiff = (std::max)(0.01, prevClose * 0.01);

    return maxDiff;
}

COLORREF CFloatingWnd::GetFluctuationColor(const STOCK::RealTimeData& realtimeData, bool hasRealtimeData) const
{
    if (!hasRealtimeData)
        return StockConstants::COLOR_NEUTRAL;

    if (realtimeData.currentPrice > realtimeData.prevClosePrice)
        return StockConstants::COLOR_RISE_TEXT;
    if (realtimeData.currentPrice < realtimeData.prevClosePrice)
        return StockConstants::COLOR_FALL_TEXT;
    return StockConstants::COLOR_NEUTRAL;
}

CRect CFloatingWnd::GetCloseButtonRect(const CRect& headerRect) const
{
    const int buttonSize = g_data.RDPI(18);
    const int rightPadding = g_data.RDPI(8);
    const int topPadding = g_data.RDPI(6);
    return CRect(headerRect.right - rightPadding - buttonSize,
                 headerRect.top + topPadding,
                 headerRect.right - rightPadding,
                 headerRect.top + topPadding + buttonSize);
}

void CFloatingWnd::DrawCloseButton(CDC* pDC, const CRect& headerRect)
{
    const CRect closeButtonRect = GetCloseButtonRect(headerRect);
    pDC->FillSolidRect(closeButtonRect, RGB(239, 243, 248));

    CPen borderPen(PS_SOLID, 1, RGB(214, 220, 228));
    CPen* oldPen = pDC->SelectObject(&borderPen);
    pDC->MoveTo(closeButtonRect.left, closeButtonRect.top);
    pDC->LineTo(closeButtonRect.right - 1, closeButtonRect.top);
    pDC->LineTo(closeButtonRect.right - 1, closeButtonRect.bottom - 1);
    pDC->LineTo(closeButtonRect.left, closeButtonRect.bottom - 1);
    pDC->LineTo(closeButtonRect.left, closeButtonRect.top);
    pDC->SelectObject(oldPen);

    pDC->SetTextColor(RGB(88, 96, 108));
    CRect textRect(closeButtonRect);
    pDC->DrawText(L"x", &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void CFloatingWnd::RequestClose()
{
    if (m_transparentWnd.GetSafeHwnd())
    {
        m_transparentWnd.PostMessage(TWND_MSG_CLOSE_OWNER, 0, 0);
    }
}

void CFloatingWnd::OnPaint()
{
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(&rect);

    // 双缓冲绘制
    CDC memDC;
    CBitmap memBitmap;
    memDC.CreateCompatibleDC(&dc);
    if (m_pFont)
    {
        memDC.SelectObject(m_pFont);
    }
    memBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap *pOldBitmap = memDC.SelectObject(&memBitmap);

    // 绘制背景
    memDC.FillSolidRect(rect, StockConstants::KLINE_BACKGROUND);
    memDC.SetBkMode(TRANSPARENT);

    const int totalH = rect.Height();
    const int w = rect.Width();
    const int headerH = (std::max)(g_data.RDPI(44), totalH / 6);

    CRect headerRect(rect.left, rect.top, rect.right, rect.top + headerH);
    CRect contentRect(rect.left, headerRect.bottom, rect.right, rect.bottom);
    if (contentRect.Height() <= 0)
        contentRect = rect;

    // 获取数据（复制快照避免长时间持锁）
    STOCK::RealTimeData realtimeData;
    std::vector<STOCK::TimelinePoint> timelinePoint;
    std::wstring stockName;
    std::wstring stockCode;
    bool hasStockData = false;
    bool hasRealtimeData = false;
    {
        std::wstring stockId;
        {
            std::lock_guard<std::mutex> lock(m_stockIdMutex);
            stockId = m_stockId;
        }

        stockCode = stockId;
        std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());
        auto stockData = g_data.GetStockData(stockId);
        if (stockData != nullptr)
        {
            hasStockData = true;
            stockCode = stockData->info.code;
            stockName = stockData->GetDisplayName();
            realtimeData = stockData->realTimeData;
            hasRealtimeData = (realtimeData.currentPrice > 0 && realtimeData.prevClosePrice > 0);
            auto timeline = stockData->getTimelineData();
            if (timeline != nullptr)
                timelinePoint = timeline->data;
        }
    }

    const bool hasTimelineData = !timelinePoint.empty();
    const std::string lastTimelineTime = hasTimelineData ? timelinePoint.back().time : "";

    DrawSummaryHeader(&memDC, headerRect, stockName, stockCode, realtimeData,
                      lastTimelineTime, hasRealtimeData, hasTimelineData);

    int savedDC = memDC.SaveDC();
    memDC.SetViewportOrg(contentRect.left, contentRect.top);
    const int contentW = contentRect.Width();
    const int contentH = contentRect.Height();
    const int timelineH = static_cast<int>(contentH * StockConstants::TIMELINE_HEIGHT_RATIO);

    DrawGrid(&memDC, contentW, timelineH, timelineH);

    if (hasTimelineData)
    {
        const STOCK::Price displayPriceLimit = CalculateDisplayPriceLimit(realtimeData, timelinePoint);
        DrawPriceLabels(&memDC, realtimeData, contentW, timelineH, displayPriceLimit);
        DrawTimelineCurve(&memDC, timelinePoint, realtimeData, 0, 0, contentW, timelineH, displayPriceLimit);
    }
    else
    {
        CString statusText;
        {
            std::lock_guard<std::mutex> lock(m_loadingTextMutex);
            statusText = m_loadingStateText;
        }

        if (statusText.IsEmpty())
        {
            statusText = g_data.StringRes(IDS_LOADING).GetString();
        }
        if (hasStockData && hasRealtimeData && !m_isThreadRunning.load())
        {
            statusText = g_data.StringRes(IDS_TIMELINE_NO_DATA);
        }
        else if (!hasStockData || !hasRealtimeData)
        {
            statusText = g_data.StringRes(IDS_WAITING_REALTIME_DATA);
        }

        CRect statusRect(0, 0, contentW, contentH);
        DrawCenteredStatusText(&memDC, statusRect, statusText, StockConstants::COLOR_NEUTRAL);
    }
    memDC.RestoreDC(savedDC);

    // 复制到屏幕
    dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBitmap);
}

void CFloatingWnd::DrawVolumeChart(CDC *pDC, const std::vector<STOCK::TimelinePoint> &timelinePoint,
                                    const std::vector<CPoint> &dataPoints, STOCK::Price prevClosePrice,
                                    int volumeTop, int volumeH, int w)
{
    if (timelinePoint.empty() || dataPoints.empty())
        return;

    // 计算最大成交量用于Y轴缩放
    STOCK::Volume maxVolume = 0;
    for (const auto &point : timelinePoint)
    {
        if (point.volume > maxVolume)
            maxVolume = point.volume;
    }

    if (maxVolume == 0)
        return;

    // 计算柱状图宽度
    int barWidth = (std::max)(1, w / static_cast<int>(timelinePoint.size()) - 1);
    if (barWidth < 1)
        barWidth = 1;

    // 绘制每个成交量柱
    for (size_t i = 0; i < timelinePoint.size() && i < dataPoints.size(); i++)
    {
        const auto &point = timelinePoint[i];
        int barHeight = static_cast<int>((static_cast<double>(point.volume) / maxVolume) * volumeH);
        if (barHeight < 1)
            barHeight = 1;

        int barX = dataPoints[i].x;
        int barY = volumeTop + volumeH - barHeight;

        // 判断涨跌颜色：当前价格与前一个价格比较，或与昨收比较
        COLORREF barColor;
        if (i == 0)
        {
            // 第一个点与昨收比较
            barColor = (point.price >= prevClosePrice) ? StockConstants::COLOR_RISE : StockConstants::COLOR_FALL;
        }
        else
        {
            // 与前一个点比较
            barColor = (point.price >= timelinePoint[i - 1].price) ? StockConstants::COLOR_RISE : StockConstants::COLOR_FALL;
        }

        // 绘制柱状
        CBrush brush(barColor);
        CBrush *pOldBrush = pDC->SelectObject(&brush);
        CPen pen(PS_SOLID, 1, barColor);
        CPen *pOldPen = pDC->SelectObject(&pen);

        pDC->Rectangle(barX - barWidth / 2, barY, barX + barWidth / 2 + 1, volumeTop + volumeH);

        pDC->SelectObject(pOldBrush);
        pDC->SelectObject(pOldPen);
    }
}

BOOL CFloatingWnd::OnEraseBkgnd(CDC *pDC)
{
    return TRUE; // 不擦除背景
}

void CFloatingWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    CRect clientRect;
    GetClientRect(&clientRect);
    const int headerH = (std::max)(g_data.RDPI(44), clientRect.Height() / 6);
    const CRect headerRect(clientRect.left, clientRect.top, clientRect.right, clientRect.top + headerH);
    if (GetCloseButtonRect(headerRect).PtInRect(point))
    {
        RequestClose();
        return;
    }

    CWnd::OnLButtonDown(nFlags, point);
}

void CFloatingWnd::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CWnd::OnActivate(nState, pWndOther, bMinimized);

    if (nState == WA_INACTIVE && !m_isDestroying.load())
    {
        RequestClose();
    }
}

void CFloatingWnd::RequestData()
{
    // 使用 compare_exchange 确保线程安全启动
    bool expected = false;
    if (m_isThreadRunning.compare_exchange_strong(expected, true))
    {
        {
            std::lock_guard<std::mutex> lock(m_loadingTextMutex);
            m_loadingStateText = g_data.StringRes(IDS_LOADING).GetString();
        }
        AfxBeginThread(NetworkThreadProc, this);
    }
}

UINT CFloatingWnd::NetworkThreadProc(LPVOID pParam)
{
    CFloatingWnd *pFW = (CFloatingWnd *)pParam;

    // 检查窗口是否正在销毁
    if (pFW == nullptr || pFW->m_isDestroying.load())
    {
        if (pFW)
            pFW->m_isThreadRunning.store(false);
        return 0;
    }

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // 使用 RAII 确保线程标志被重置
    struct ThreadGuard {
        std::atomic<bool>& flag;
        ~ThreadGuard() { flag.store(false); }
    } guard{pFW->m_isThreadRunning};

    // 再次检查，防止在获取锁期间窗口被销毁
    if (pFW->m_isDestroying.load())
    {
        return 0;
    }

    // 线程安全地获取 stockId
    std::wstring stockId;
    {
        std::lock_guard<std::mutex> lock(pFW->m_stockIdMutex);
        stockId = pFW->m_stockId;
    }

    if (stockId.empty())
    {
        return 0;
    }

    g_data.RequestTimelineData(stockId);

    return 0;
}
