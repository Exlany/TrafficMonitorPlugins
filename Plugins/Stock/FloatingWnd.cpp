#include "pch.h"
#include "FloatingWnd.h"
#include "StockConstants.h"
#include <afxinet.h>
#include <memory>
#include <algorithm>
#include "Common.h"
#include "DataManager.h"

// 使用显式命名空间限定代替 using namespace

BEGIN_MESSAGE_MAP(CFloatingWnd, CWnd)
ON_WM_PAINT()
ON_WM_ERASEBKGND()
ON_WM_LBUTTONDOWN()
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
    if (!m_transparentWnd.CreateEx(WS_EX_TOOLWINDOW /* | WS_EX_LAYERED */ /* | WS_EX_TRANSPARENT */,
                                    L"CTransparentWnd", L"", WS_POPUP | WS_VISIBLE,
                                    screenRect, NULL, 0, NULL))
    {
        TRACE(L"Failed to create transparent window\n");
        return FALSE;
    }

    const SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const int WIDTH = g_data.RDPI(settings.klineWidth);
    const int HEIGHT = g_data.RDPI(settings.klineHeight);
    int x = pt.x;
    int y = pt.y;

    // 调整位置
    if (x + WIDTH > screenRect.right)
        x = x - WIDTH;
    if (y + HEIGHT > screenRect.bottom)
        y = y - HEIGHT;
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

    // 设置完全透明
    m_transparentWnd.SetLayeredWindowAttributes(0, 0, LWA_ALPHA);
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

void CFloatingWnd::DrawPriceLabels(CDC *pDC, const STOCK::RealTimeData &realtimeData, int w, int timelineH)
{
    STOCK::Price priceLimit = realtimeData.priceLimit;
    CRect timelineRect(0, 0, w, timelineH);

    pDC->SetTextColor(StockConstants::COLOR_RISE_TEXT);
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

    pDC->SetTextColor(StockConstants::COLOR_FALL_TEXT);
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

    pDC->SetTextColor(StockConstants::COLOR_NEUTRAL);
    CString middleTxt;
    middleTxt.Format(_T("%.2f"), realtimeData.prevClosePrice);
    CRect middleTxtRect{timelineRect};
    middleTxtRect.right = middleTxtRect.left + pDC->GetTextExtent(middleTxt).cx;
    pDC->DrawText(middleTxt, middleTxtRect, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void CFloatingWnd::DrawTimelineCurve(CDC *pDC, const std::vector<STOCK::TimelinePoint> &timelinePoint,
                                      const STOCK::RealTimeData &realtimeData, int x, int y, int w, int h)
{
    float halfH = h / 2.0f;
    STOCK::Price priceLimit = realtimeData.priceLimit;
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

    int x = rect.left, y = rect.top, totalH = rect.Height(), w = rect.Width();

    // 分区计算
    int timelineH = static_cast<int>(totalH * StockConstants::TIMELINE_HEIGHT_RATIO);
    int h = timelineH;

    // 绘制网格
    DrawGrid(&memDC, w, h, timelineH);

    // 获取数据
    STOCK::RealTimeData realtimeData;
    std::vector<STOCK::TimelinePoint> timelinePoint;
    {
        // 线程安全：先获取 stockId 的副本
        std::wstring stockId;
        {
            std::lock_guard<std::mutex> lock(m_stockIdMutex);
            stockId = m_stockId;
        }

        std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());
        auto stockData = g_data.GetStockData(stockId);
        if (stockData == nullptr)
        {
            dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
            memDC.SelectObject(pOldBitmap);
            return;
        }
        realtimeData = stockData->realTimeData;
        auto timeline = stockData->getTimelineData();
        if (timeline != nullptr)
            timelinePoint = timeline->data;
    }

    if (!timelinePoint.empty())
    {
        DrawPriceLabels(&memDC, realtimeData, w, timelineH);
        DrawTimelineCurve(&memDC, timelinePoint, realtimeData, x, y, w, h);
    }
    else
    {
        CString loadingText;
        {
            std::lock_guard<std::mutex> lock(m_loadingTextMutex);
            loadingText = m_loadingStateText;
        }
        memDC.SetTextColor(StockConstants::COLOR_NEUTRAL);
        memDC.TextOut((w - memDC.GetTextExtent(loadingText).cx) / 2, g_data.RDPI(10), loadingText);
    }

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
