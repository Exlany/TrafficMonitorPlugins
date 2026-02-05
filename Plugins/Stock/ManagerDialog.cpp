// ManagerDialog.cpp: 实现文件
//

#include "pch.h"
#include "Stock.h"
#include "afxdialogex.h"
#include "ManagerDialog.h"
#include "Common.h"
#include "OptionsDlg.h"
#include <Windows.h>

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
ON_BN_CLICKED(IDC_FULL_DAY_CHECK, &CManagerDialog::OnClickedFullDayCheck)
ON_BN_CLICKED(IDOK, &CManagerDialog::OnBnClickedOk)
ON_BN_CLICKED(IDCANCEL, &CManagerDialog::OnBnClickedCancel)
ON_LBN_DBLCLK(IDC_MGR_LIST, &CManagerDialog::OnLbnDblclkMgrList)
ON_WM_GETMINMAXINFO()
ON_BN_CLICKED(IDC_SHOW_STOCK_NAME_CHECK, &CManagerDialog::OnBnClickedShowStockNameCheck)
ON_BN_CLICKED(IDC_COLOR_WITH_PRICE_CHECK, &CManagerDialog::OnBnClickedColorWithPriceCheck)
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

    for (const auto &stock_code : m_data.m_stock_codes)
    {
        m_stock_listbox.AddString(stock_code.c_str());
    }

    for (int i = 0; i < m_stock_listbox.GetCount(); i++)
    {
        m_stock_listbox.SetItemHeight(i, g_data.DPI(20));
    }

    CheckDlgButton(IDC_FULL_DAY_CHECK, m_data.m_full_day);
    CheckDlgButton(IDC_SHOW_STOCK_NAME_CHECK, m_data.m_show_stock_name);
    CheckDlgButton(IDC_COLOR_WITH_PRICE_CHECK, m_data.m_color_with_price);

    // 初始化价格小数位数下拉框
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_PRICE_DECIMAL_COMBO);
    pCombo->AddString(_T("2"));
    pCombo->AddString(_T("3"));
    pCombo->SetCurSel(m_data.m_price_decimal == 2 ? 0 : 1);

    // 初始化显示模式下拉框
    CComboBox* pModeCombo = (CComboBox*)GetDlgItem(IDC_DISPLAY_MODE_COMBO);
    pModeCombo->AddString(g_data.StringRes(IDS_DISPLAY_MODE_SHOWALL));
    pModeCombo->AddString(g_data.StringRes(IDS_DISPLAY_MODE_CAROUSEL));
    pModeCombo->AddString(g_data.StringRes(IDS_DISPLAY_MODE_MANUAL));
    pModeCombo->AddString(g_data.StringRes(IDS_DISPLAY_MODE_SMART));
    pModeCombo->SetCurSel(static_cast<int>(m_data.m_display_mode));

    // 初始化轮播间隔
    CString intervalStr;
    intervalStr.Format(_T("%d"), m_data.m_carousel_interval);
    SetDlgItemText(IDC_CAROUSEL_INTERVAL_EDIT, intervalStr);

    CString value;
    value.Format(_T("%d"), static_cast<int>(g_data.m_setting_data.m_kline_width));
    SetDlgItemText(IDC_KLINE_WIDTH_EDIT, value);
    value.Format(_T("%d"), static_cast<int>(g_data.m_setting_data.m_kline_height));
    SetDlgItemText(IDC_KLINE_HEIGHT_EDIT, value);

    return TRUE; // return TRUE unless you set the focus to a control
                 // 异常: OCX 属性页应返回 FALSE
}

void CManagerDialog::OnListItemClick()
{
    CString curSelTxt;
    int curSelPos;

    curSelPos = m_stock_listbox.GetCurSel();
    m_stock_listbox.GetText(curSelPos, curSelTxt);
    Log1("OnListItemClick: %s\n", curSelTxt.GetString());
}

void CManagerDialog::OnDelBtnClick()
{
    int curSelPos = m_stock_listbox.GetCurSel();
    Log1("OnDelBtnClick: %d\n", curSelPos);
    if (curSelPos < 0 || curSelPos > m_data.m_stock_codes.size())
    {
        return;
    }
    m_stock_listbox.DeleteString(curSelPos);
    m_data.m_stock_codes.erase(m_data.m_stock_codes.begin() + curSelPos);
}

void CManagerDialog::OnAddBtnClick()
{
    if (m_data.m_stock_codes.size() >= Stock_ITEM_MAX)
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
            if (count(m_data.m_stock_codes.begin(), m_data.m_stock_codes.end(), stock_code))
            {
                Log1("OnAddBtnClick: ignore %s\n", stock_code.c_str());
                return;
            }
            Log1("OnAddBtnClick: %s\n", stock_code.c_str());
            m_data.m_stock_codes.push_back(stock_code.c_str());
            m_stock_listbox.AddString(stock_code.c_str());

            // 保存别名
            std::wstring alias = dlg.m_stock_alias.GetString();
            if (!alias.empty())
            {
                m_data.m_stock_aliases[stock_code] = alias;
            }
        }
    }
}

void CManagerDialog::OnClickedFullDayCheck()
{
    m_data.m_full_day = (IsDlgButtonChecked(IDC_FULL_DAY_CHECK) != 0);
}

void CManagerDialog::OnBnClickedShowStockNameCheck()
{
    m_data.m_show_stock_name = (IsDlgButtonChecked(IDC_SHOW_STOCK_NAME_CHECK) != 0);
}

void CManagerDialog::OnBnClickedColorWithPriceCheck()
{
    m_data.m_color_with_price = (IsDlgButtonChecked(IDC_COLOR_WITH_PRICE_CHECK) != 0);
}

void CManagerDialog::OnBnClickedOk()
{
    bool stock_code_changed{g_data.m_setting_data.m_stock_codes != m_data.m_stock_codes};
    CString value;
    GetDlgItemText(IDC_KLINE_WIDTH_EDIT, value);
    m_data.m_kline_width = _ttoi(value);
    GetDlgItemText(IDC_KLINE_HEIGHT_EDIT, value);
    m_data.m_kline_height = _ttoi(value);

    // 保存价格小数位数
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_PRICE_DECIMAL_COMBO);
    m_data.m_price_decimal = (pCombo->GetCurSel() == 0) ? 2 : 3;

    // 保存显示模式
    CComboBox* pModeCombo = (CComboBox*)GetDlgItem(IDC_DISPLAY_MODE_COMBO);
    m_data.m_display_mode = static_cast<StockDisplayMode>(pModeCombo->GetCurSel());

    // 保存轮播间隔
    CString intervalStr;
    GetDlgItemText(IDC_CAROUSEL_INTERVAL_EDIT, intervalStr);
    m_data.m_carousel_interval = _ttoi(intervalStr);
    if (m_data.m_carousel_interval < 1)
        m_data.m_carousel_interval = 1;  // 最小1秒

    g_data.m_setting_data = m_data;
    g_data.SaveConfig();
    if (stock_code_changed)
    {
        Stock::Instance().SendStockInfoRequest();
        MessageBox(g_data.StringRes(IDS_CHANGE_STOCK_TIP), g_data.StringRes(IDS_PLUGIN_NAME), MB_ICONINFORMATION | MB_OK);
    }
    CDialog::OnOK();
}

void CManagerDialog::OnBnClickedCancel()
{
    CDialog::OnCancel();
}

void CManagerDialog::OnLbnDblclkMgrList()
{
    int index = m_stock_listbox.GetCurSel();
    if (index >= 0 && index < m_data.m_stock_codes.size())
    {
        std::wstring old_code = m_data.m_stock_codes[index];
        COptionsDlg dlg(old_code, this);
        auto rtn = dlg.DoModal();
        if (rtn == IDOK)
        {
            if (!dlg.m_stock_code.IsEmpty())
            {
                std::wstring new_code = dlg.m_stock_code.GetString();
                m_data.m_stock_codes[index] = new_code;
                m_stock_listbox.DeleteString(index);
                m_stock_listbox.InsertString(index, dlg.m_stock_code);

                // 更新别名
                m_data.m_stock_aliases.erase(old_code);
                std::wstring alias = dlg.m_stock_alias.GetString();
                if (!alias.empty())
                {
                    m_data.m_stock_aliases[new_code] = alias;
                }
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
