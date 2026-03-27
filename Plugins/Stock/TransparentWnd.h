#pragma once
#include <afxwin.h>

#define TWND_MSG_CLOSE_OWNER (WM_USER + 180)

class CTransparentWnd : public CWnd
{
public:
    CTransparentWnd();
    void SetParent(CWnd *pParent) { m_pParent = pParent; } // 添加设置父窗口的方法

protected:
    DECLARE_MESSAGE_MAP()
    //afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnRequestCloseOwner(WPARAM wParam, LPARAM lParam);

private:
    CWnd *m_pParent;
};
