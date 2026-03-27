// ManagerDialog.cpp: 实现文件
//

#include "pch.h"
#include "Stock.h"
#include "StockConstants.h"
#include "afxdialogex.h"
#include "ManagerDialog.h"
#include "Common.h"
#include "OptionsDlg.h"
#include <Windows.h>
#include <algorithm>

namespace
{
template<typename T>
T ClampValue(T value, T minValue, T maxValue, T fallback)
{
    if (value < minValue || value > maxValue)
        return fallback;
    return value;
}
}

// CManagerDialog 对话框

IMPLEMENT_DYNAMIC(CManagerDialog, CDialog)

CManagerDialog::CManagerDialog(CWnd *pParent /*=nullptr*/)
    : CDialog(IDD_MANAGER_DIALOG, pParent)
{
}

CManagerDialog::~CManagerDialog()
{
}

void CManagerDialog::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MGR_LIST, m_stock_listbox);
}

BEGIN_MESSAGE_MAP(CManagerDialog, CDialog)
ON_LBN_SELCHANGE(IDC_MGR_LIST, &CManagerDialog::OnListItemClick)
ON_BN_CLICKED(IDC_MGR_DEL_BTN, &CManagerDialog::OnDelBtnClick)
ON_BN_CLICKED(IDC_MGR_ADD_BTN, &CManagerDialog::OnAddBtnClick)
ON_BN_CLICKED(IDC_MGR_UP_BTN, &CManagerDialog::OnMoveUpBtnClick)
ON_BN_CLICKED(IDC_MGR_DOWN_BTN, &CManagerDialog::OnMoveDownBtnClick)
ON_BN_CLICKED(IDC_FULL_DAY_CHECK, &CManagerDialog::OnClickedFullDayCheck)
ON_BN_CLICKED(IDOK, &CManagerDialog::OnBnClickedOk)
ON_BN_CLICKED(IDCANCEL, &CManagerDialog::OnBnClickedCancel)
ON_LBN_DBLCLK(IDC_MGR_LIST, &CManagerDialog::OnLbnDblclkMgrList)
ON_WM_GETMINMAXINFO()
ON_BN_CLICKED(IDC_SHOW_STOCK_NAME_CHECK, &CManagerDialog::OnBnClickedShowStockNameCheck)
ON_BN_CLICKED(IDC_COLOR_WITH_PRICE_CHECK, &CManagerDialog::OnBnClickedColorWithPriceCheck)
ON_BN_CLICKED(IDC_ENABLE_PRICE_ALERT_CHECK, &CManagerDialog::OnBnClickedEnablePriceAlertCheck)
ON_CBN_SELCHANGE(IDC_DISPLAY_MODE_COMBO, &CManagerDialog::OnCbnSelchangeDisplayModeCombo)
END_MESSAGE_MAP()

// CManagerDialog 消息处理程序

BOOL CManagerDialog::OnInitDialog()
{
    CDialog::OnInitDialog();
    HICON hIcon = g_data.GetIcon(IDI_STOCK);
    SetIcon(hIcon, FALSE);

    // 获取初始时窗口的大小
    CRect rect;
    GetWindowRect(rect);
    m_min_size.cx = rect.Width();
    m_min_size.cy = rect.Height();

    RefreshStockList();

    CheckDlgButton(IDC_FULL_DAY_CHECK, m_data.fullDay);
    CheckDlgButton(IDC_SHOW_STOCK_NAME_CHECK, m_data.showStockName);
    CheckDlgButton(IDC_COLOR_WITH_PRICE_CHECK, m_data.colorWithPrice);
    CheckDlgButton(IDC_SHOW_STATUS_MARKER_CHECK, m_data.showStatusMarker);
    CheckDlgButton(IDC_ENABLE_PRICE_ALERT_CHECK, m_data.enablePriceAlert);

    // 初始化价格小数位数下拉框
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_PRICE_DECIMAL_COMBO);
    pCombo->AddString(_T("2"));
    pCombo->AddString(_T("3"));
    pCombo->SetCurSel(m_data.priceDecimal == 2 ? 0 : 1);

    // 初始化显示模式下拉框
    CComboBox* pModeCombo = (CComboBox*)GetDlgItem(IDC_DISPLAY_MODE_COMBO);
    pModeCombo->AddString(g_data.StringRes(IDS_DISPLAY_MODE_SHOWALL));
    pModeCombo->AddString(g_data.StringRes(IDS_DISPLAY_MODE_CAROUSEL));
    pModeCombo->AddString(g_data.StringRes(IDS_DISPLAY_MODE_MANUAL));
    pModeCombo->AddString(g_data.StringRes(IDS_DISPLAY_MODE_SMART));
    pModeCombo->SetCurSel(static_cast<int>(m_data.displayMode));

    // 初始化轮播间隔
    CString intervalStr;
    intervalStr.Format(_T("%d"), m_data.carouselInterval);
    SetDlgItemText(IDC_CAROUSEL_INTERVAL_EDIT, intervalStr);
    intervalStr.Format(_T("%d"), m_data.alertChangePercent);
    SetDlgItemText(IDC_ALERT_THRESHOLD_EDIT, intervalStr);
    intervalStr.Format(_T("%d"), m_data.tooltipMaxItems);
    SetDlgItemText(IDC_TOOLTIP_MAX_ITEMS_EDIT, intervalStr);

    CString value;
    value.Format(_T("%d"), static_cast<int>(m_data.klineWidth));
    SetDlgItemText(IDC_KLINE_WIDTH_EDIT, value);
    value.Format(_T("%d"), static_cast<int>(m_data.klineHeight));
    SetDlgItemText(IDC_KLINE_HEIGHT_EDIT, value);
    CWnd* pWidthEdit = GetDlgItem(IDC_KLINE_WIDTH_EDIT);
    if (pWidthEdit != nullptr)
        pWidthEdit->SendMessage(EM_LIMITTEXT, 4, 0);
    CWnd* pHeightEdit = GetDlgItem(IDC_KLINE_HEIGHT_EDIT);
    if (pHeightEdit != nullptr)
        pHeightEdit->SendMessage(EM_LIMITTEXT, 4, 0);
    CWnd* pCarouselEdit = GetDlgItem(IDC_CAROUSEL_INTERVAL_EDIT);
    if (pCarouselEdit != nullptr)
        pCarouselEdit->SendMessage(EM_LIMITTEXT, 2, 0);
    CWnd* pAlertEdit = GetDlgItem(IDC_ALERT_THRESHOLD_EDIT);
    if (pAlertEdit != nullptr)
        pAlertEdit->SendMessage(EM_LIMITTEXT, 2, 0);
    CWnd* pTooltipEdit = GetDlgItem(IDC_TOOLTIP_MAX_ITEMS_EDIT);
    if (pTooltipEdit != nullptr)
        pTooltipEdit->SendMessage(EM_LIMITTEXT, 2, 0);
    UpdateDisplayModeControls();
    UpdateAlertControls();
    UpdateActionButtonState();

    return TRUE; // return TRUE unless you set the focus to a control
                 // 异常: OCX 属性页应返回 FALSE
}

void CManagerDialog::OnListItemClick()
{
    UpdateActionButtonState();
}

void CManagerDialog::OnDelBtnClick()
{
    int curSelPos = m_stock_listbox.GetCurSel();
    TRACE(L"OnDelBtnClick: %d\n", curSelPos);
    if (curSelPos < 0 || static_cast<size_t>(curSelPos) >= m_data.stockCodes.size())
    {
        return;
    }
    m_data.aliases.erase(m_data.stockCodes[curSelPos]);
    m_stock_listbox.DeleteString(curSelPos);
    m_data.stockCodes.erase(m_data.stockCodes.begin() + curSelPos);
    RefreshStockList((std::min)(curSelPos, m_stock_listbox.GetCount() - 1));
}

void CManagerDialog::OnAddBtnClick()
{
    if (m_data.stockCodes.size() >= StockConstants::MAX_STOCK_ITEMS)
    {
        MessageBox(g_data.StringRes(IDS_STOCK_NUM_LIMIT_WARNING), g_data.StringRes(IDS_PLUGIN_NAME), MB_ICONWARNING | MB_OK);
        return;
    }
    COptionsDlg dlg(std::wstring(), this);
    auto rtn = dlg.DoModal();
    if (rtn == IDOK)
    {
        std::wstring stock_code = dlg.m_stock_code.GetString();
        if (!stock_code.empty())
        {
            auto duplicate = std::find(m_data.stockCodes.begin(), m_data.stockCodes.end(), stock_code);
            if (duplicate != m_data.stockCodes.end())
            {
                m_stock_listbox.SetCurSel(static_cast<int>(std::distance(m_data.stockCodes.begin(), duplicate)));
                UpdateActionButtonState();
                return;
            }
            TRACE(L"OnAddBtnClick: %s\n", stock_code.c_str());
            m_data.stockCodes.push_back(stock_code.c_str());

            // 保存别名
            std::wstring alias = dlg.m_stock_alias.GetString();
            if (!alias.empty())
            {
                m_data.aliases[stock_code] = alias;
            }
            else
            {
                m_data.aliases.erase(stock_code);
            }
            RefreshStockList(static_cast<int>(m_data.stockCodes.size()) - 1);
        }
    }
}

void CManagerDialog::OnMoveUpBtnClick()
{
    const int curSelPos = m_stock_listbox.GetCurSel();
    if (curSelPos <= 0 || static_cast<size_t>(curSelPos) >= m_data.stockCodes.size())
    {
        return;
    }

    std::swap(m_data.stockCodes[curSelPos], m_data.stockCodes[curSelPos - 1]);
    RefreshStockList(curSelPos - 1);
}

void CManagerDialog::OnMoveDownBtnClick()
{
    const int curSelPos = m_stock_listbox.GetCurSel();
    if (curSelPos < 0 || static_cast<size_t>(curSelPos + 1) >= m_data.stockCodes.size())
    {
        return;
    }

    std::swap(m_data.stockCodes[curSelPos], m_data.stockCodes[curSelPos + 1]);
    RefreshStockList(curSelPos + 1);
}

void CManagerDialog::OnClickedFullDayCheck()
{
    m_data.fullDay = (IsDlgButtonChecked(IDC_FULL_DAY_CHECK) != 0);
}

void CManagerDialog::OnBnClickedShowStockNameCheck()
{
    m_data.showStockName = (IsDlgButtonChecked(IDC_SHOW_STOCK_NAME_CHECK) != 0);
}

void CManagerDialog::OnBnClickedColorWithPriceCheck()
{
    m_data.colorWithPrice = (IsDlgButtonChecked(IDC_COLOR_WITH_PRICE_CHECK) != 0);
}

void CManagerDialog::OnBnClickedEnablePriceAlertCheck()
{
    UpdateAlertControls();
}

void CManagerDialog::OnBnClickedOk()
{
    CString value;
    GetDlgItemText(IDC_KLINE_WIDTH_EDIT, value);
    m_data.klineWidth = ClampValue<unsigned int>(_ttoi(value), 100, 2000, StockConstants::DEFAULT_KLINE_WIDTH);
    GetDlgItemText(IDC_KLINE_HEIGHT_EDIT, value);
    m_data.klineHeight = ClampValue<unsigned int>(_ttoi(value), 50, 1000, StockConstants::DEFAULT_KLINE_HEIGHT);

    // 保存价格小数位数
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_PRICE_DECIMAL_COMBO);
    m_data.priceDecimal = (pCombo->GetCurSel() == 0) ? 2 : 3;

    // 保存显示模式
    CComboBox* pModeCombo = (CComboBox*)GetDlgItem(IDC_DISPLAY_MODE_COMBO);
    m_data.displayMode = static_cast<StockDisplayMode>(pModeCombo->GetCurSel());

    // 保存轮播间隔
    CString intervalStr;
    GetDlgItemText(IDC_CAROUSEL_INTERVAL_EDIT, intervalStr);
    m_data.carouselInterval = ClampValue<int>(_ttoi(intervalStr), 1, 60, StockConstants::DEFAULT_CAROUSEL_INTERVAL);
    m_data.showStatusMarker = (IsDlgButtonChecked(IDC_SHOW_STATUS_MARKER_CHECK) != 0);
    m_data.enablePriceAlert = (IsDlgButtonChecked(IDC_ENABLE_PRICE_ALERT_CHECK) != 0);

    CString thresholdStr;
    GetDlgItemText(IDC_ALERT_THRESHOLD_EDIT, thresholdStr);
    m_data.alertChangePercent = ClampValue<int>(_ttoi(thresholdStr),
        StockConstants::MIN_ALERT_CHANGE_PERCENT,
        StockConstants::MAX_ALERT_CHANGE_PERCENT,
        StockConstants::DEFAULT_ALERT_CHANGE_PERCENT);

    CString tooltipMaxItemsStr;
    GetDlgItemText(IDC_TOOLTIP_MAX_ITEMS_EDIT, tooltipMaxItemsStr);
    m_data.tooltipMaxItems = ClampValue<int>(_ttoi(tooltipMaxItemsStr),
        StockConstants::MIN_TOOLTIP_MAX_ITEMS,
        StockConstants::MAX_TOOLTIP_MAX_ITEMS,
        StockConstants::DEFAULT_TOOLTIP_MAX_ITEMS);

    value.Format(_T("%u"), m_data.klineWidth);
    SetDlgItemText(IDC_KLINE_WIDTH_EDIT, value);
    value.Format(_T("%u"), m_data.klineHeight);
    SetDlgItemText(IDC_KLINE_HEIGHT_EDIT, value);
    intervalStr.Format(_T("%d"), m_data.carouselInterval);
    SetDlgItemText(IDC_CAROUSEL_INTERVAL_EDIT, intervalStr);
    thresholdStr.Format(_T("%d"), m_data.alertChangePercent);
    SetDlgItemText(IDC_ALERT_THRESHOLD_EDIT, thresholdStr);
    tooltipMaxItemsStr.Format(_T("%d"), m_data.tooltipMaxItems);
    SetDlgItemText(IDC_TOOLTIP_MAX_ITEMS_EDIT, tooltipMaxItemsStr);

    CDialog::OnOK();
}

void CManagerDialog::OnBnClickedCancel()
{
    CDialog::OnCancel();
}

void CManagerDialog::OnLbnDblclkMgrList()
{
    int index = m_stock_listbox.GetCurSel();
    if (index >= 0 && static_cast<size_t>(index) < m_data.stockCodes.size())
    {
        std::wstring old_code = m_data.stockCodes[index];
        COptionsDlg dlg(old_code, this);
        auto rtn = dlg.DoModal();
        if (rtn == IDOK)
        {
            if (!dlg.m_stock_code.IsEmpty())
            {
                std::wstring new_code = dlg.m_stock_code.GetString();
                auto duplicate = std::find(m_data.stockCodes.begin(), m_data.stockCodes.end(), new_code);
                if (duplicate != m_data.stockCodes.end() && static_cast<size_t>(std::distance(m_data.stockCodes.begin(), duplicate)) != static_cast<size_t>(index))
                {
                    m_stock_listbox.SetCurSel(static_cast<int>(std::distance(m_data.stockCodes.begin(), duplicate)));
                    UpdateActionButtonState();
                    return;
                }
                m_data.stockCodes[index] = new_code;

                // 更新别名
                m_data.aliases.erase(old_code);
                std::wstring alias = dlg.m_stock_alias.GetString();
                if (!alias.empty())
                {
                    m_data.aliases[new_code] = alias;
                }
                RefreshStockList(index);
            }
        }
    }
}

void CManagerDialog::OnGetMinMaxInfo(MINMAXINFO *lpMMI)
{
    // 限制窗口最小大小
    lpMMI->ptMinTrackSize.x = m_min_size.cx; // 设置最小宽度
    lpMMI->ptMinTrackSize.y = m_min_size.cy; // 设置最小高度

    CDialog::OnGetMinMaxInfo(lpMMI);
}

void CManagerDialog::RefreshStockList(int selectedIndex)
{
    m_stock_listbox.ResetContent();
    for (const auto& stock_code : m_data.stockCodes)
    {
        m_stock_listbox.AddString(FormatStockListEntry(stock_code));
    }

    for (int i = 0; i < m_stock_listbox.GetCount(); ++i)
    {
        m_stock_listbox.SetItemHeight(i, g_data.DPI(20));
    }

    if (selectedIndex >= 0 && selectedIndex < m_stock_listbox.GetCount())
    {
        m_stock_listbox.SetCurSel(selectedIndex);
    }
    UpdateDisplayModeControls();
    UpdateActionButtonState();
}

void CManagerDialog::UpdateDisplayModeControls()
{
    CComboBox* pModeCombo = (CComboBox*)GetDlgItem(IDC_DISPLAY_MODE_COMBO);
    if (pModeCombo == nullptr)
        return;

    const bool enableCarouselInterval = (pModeCombo->GetCurSel() == static_cast<int>(StockDisplayMode::Carousel) &&
        m_data.stockCodes.size() > 1);
    CWnd* pIntervalEdit = GetDlgItem(IDC_CAROUSEL_INTERVAL_EDIT);
    if (pIntervalEdit != nullptr)
    {
        pIntervalEdit->EnableWindow(enableCarouselInterval);
    }
}

void CManagerDialog::UpdateActionButtonState()
{
    const int curSel = m_stock_listbox.GetCurSel();
    const int count = m_stock_listbox.GetCount();
    const bool hasSelection = (curSel >= 0 && curSel < count);

    CWnd* pDeleteButton = GetDlgItem(IDC_MGR_DEL_BTN);
    if (pDeleteButton != nullptr)
    {
        pDeleteButton->EnableWindow(hasSelection);
    }

    CWnd* pUpButton = GetDlgItem(IDC_MGR_UP_BTN);
    if (pUpButton != nullptr)
    {
        pUpButton->EnableWindow(hasSelection && curSel > 0);
    }

    CWnd* pDownButton = GetDlgItem(IDC_MGR_DOWN_BTN);
    if (pDownButton != nullptr)
    {
        pDownButton->EnableWindow(hasSelection && curSel < count - 1);
    }
}

void CManagerDialog::UpdateAlertControls()
{
    const bool enableAlert = (IsDlgButtonChecked(IDC_ENABLE_PRICE_ALERT_CHECK) != 0);
    CWnd* pThresholdEdit = GetDlgItem(IDC_ALERT_THRESHOLD_EDIT);
    if (pThresholdEdit != nullptr)
    {
        pThresholdEdit->EnableWindow(enableAlert);
    }
}

CString CManagerDialog::FormatStockListEntry(const std::wstring& code) const
{
    auto aliasIt = m_data.aliases.find(code);
    if (aliasIt != m_data.aliases.end() && !aliasIt->second.empty())
    {
        CString text;
        text.Format(_T("%s (%s)"), aliasIt->second.c_str(), code.c_str());
        return text;
    }
    return code.c_str();
}

void CManagerDialog::OnCbnSelchangeDisplayModeCombo()
{
    UpdateDisplayModeControls();
}
