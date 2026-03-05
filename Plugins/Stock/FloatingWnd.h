#pragma once

#include <StockDef.h>
#include <TransparentWnd.h>
#include <atomic>
#include <mutex>

// 定义自定义消息
#define FWND_MSG_UPDATE_STATUS (WM_USER + 100)
#define FWND_MSG_REQUEST_DATA (WM_USER + 101)

class CFloatingWnd : public CWnd
{
public:
    CFloatingWnd();
    virtual ~CFloatingWnd();

    BOOL Create(CFont *font, CPoint pt, std::wstring stock_id);
    void RequestData();

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    LRESULT OnUpdateStatus(WPARAM wParam, LPARAM lParam);
    LRESULT OnRequestData(WPARAM wParam, LPARAM lParam);

private:
    static UINT NetworkThreadProc(LPVOID pParam); // 线程函数
    CPoint Stock2Point(int x, int y, int w, int h, float unitY, const STOCK::TimelinePoint &item, const STOCK::Price prevClosePrice);
    void DrawGrid(CDC *pDC, int w, int h, int timelineH);
    void DrawPriceLabels(CDC *pDC, const STOCK::RealTimeData &realtimeData, int w, int timelineH);
    void DrawTimelineCurve(CDC *pDC, const std::vector<STOCK::TimelinePoint> &timelinePoint,
                           const STOCK::RealTimeData &realtimeData, int x, int y, int w, int h);
    void DrawVolumeChart(CDC *pDC, const std::vector<STOCK::TimelinePoint> &timelinePoint,
                         const std::vector<CPoint> &dataPoints, STOCK::Price prevClosePrice,
                         int volumeTop, int volumeH, int w);

    CTransparentWnd m_transparentWnd;
    std::wstring m_stockId;
    std::atomic<bool> m_isThreadRunning{};
    std::atomic<bool> m_isDestroying{false};
    CFont *m_pFont{};
    CString m_loadingStateText;
    std::mutex m_loadingTextMutex;
    std::mutex m_stockIdMutex;

    std::atomic<unsigned __int64> m_last_request_time{};
};
