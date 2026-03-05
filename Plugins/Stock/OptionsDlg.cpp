// OptionsDlg.cpp: 实现文件
//

#include "pch.h"
#include "Stock.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"
#include "DataManager.h"
#include "Common.h"

using namespace StockConstants;

// 支持的市场类型集合
static const std::vector<CString> StockTypeSet{kSH, kSZ, kHK, kMG, kMGI, kBJ, kOKX, kBN, kFX, kHF};

// COptionsDlg 对话框

IMPLEMENT_DYNAMIC(COptionsDlg, CDialog)

COptionsDlg::COptionsDlg(const std::wstring& code, CWnd* pParent /*=nullptr*/)
    : CDialog(IDD_OPTIONS_DIALOG, pParent)
    , m_stock_code(code.c_str())
    , m_radio_stock_types(0)
{
    // 加载已有的别名
    if (!code.empty())
    {
        SettingsSnapshot settings = g_data.GetSettingsSnapshot();
        std::wstring alias = settings.GetAlias(code);
        if (!alias.empty())
        {
            m_stock_alias = alias.c_str();
        }
    }
}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::EnableUpdateBtn(bool enable)
{
    CWnd* pBtn = GetDlgItem(IDC_UPDATE_BUTTON);
    if (pBtn != nullptr && pBtn->GetSafeHwnd() != NULL)
        pBtn->EnableWindow(enable);
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CODE_EDIT, m_code_edit);
    DDX_Radio(pDX, IDC_RADIO_SZ, m_radio_stock_types);
    DDV_MinMaxInt(pDX, m_radio_stock_types, 0, 6);
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
    ON_EN_CHANGE(IDC_CODE_EDIT, &COptionsDlg::OnChangeCodeEdit)
    ON_BN_CLICKED(IDOK, &COptionsDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &COptionsDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_RADIO_SZ, &COptionsDlg::OnRadioClickedStockTypes)
    ON_BN_CLICKED(IDC_RADIO_HK, &COptionsDlg::OnRadioClickedStockTypes)
    ON_BN_CLICKED(IDC_RADIO_BJ, &COptionsDlg::OnRadioClickedStockTypes)
    ON_BN_CLICKED(IDC_RADIO_SH, &COptionsDlg::OnRadioClickedStockTypes)
    ON_BN_CLICKED(IDC_RADIO_GB, &COptionsDlg::OnRadioClickedStockTypes)
    ON_BN_CLICKED(IDC_RADIO_OKX, &COptionsDlg::OnRadioClickedStockTypes)
    ON_BN_CLICKED(IDC_RADIO_OTHER, &COptionsDlg::OnRadioClickedStockTypes)
END_MESSAGE_MAP()


// COptionsDlg 消息处理程序


BOOL COptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //设置标题
    if (m_stock_code.IsEmpty())
        SetWindowText(g_data.StringRes(IDS_ADD_STOCK));
    else
        SetWindowText(g_data.StringRes(IDS_EDIT_STOCK));

    if (!m_stock_code.IsEmpty())
    {
        CString type = GetCodeType(m_stock_code);
        if (type == kSZ)
            m_radio_stock_types = 0;
        else if (type == kHK)
            m_radio_stock_types = 1;
        else if (type == kBJ)
            m_radio_stock_types = 2;
        else if (type == kSH)
            m_radio_stock_types = 3;
        else if (type == kMG)
            m_radio_stock_types = 4;
        else if (type == kOKX)
            m_radio_stock_types = 5;
        else
            m_radio_stock_types = 6;
        UpdateData(FALSE);
    }

    RemoveTypeFromCode(m_stock_code);
    SetDlgItemText(IDC_CODE_EDIT, m_stock_code);
    SetDlgItemText(IDC_ALIAS_EDIT, m_stock_alias);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // 异常: OCX 属性页应返回 FALSE
}


void COptionsDlg::OnChangeCodeEdit()
{
    // 自动识别股票类型
    CString code;
    GetDlgItemText(IDC_CODE_EDIT, code);
    if (!code.IsEmpty())
    {
        int detectedType = AutoDetectStockType(code);
        if (detectedType != m_radio_stock_types)
        {
            m_radio_stock_types = detectedType;
            UpdateData(FALSE);
        }
    }
}

void COptionsDlg::RemoveTypeFromCode(CString& code)
{
    for (const auto& type : StockTypeSet)
    {
        if (code.Left(type.GetLength()) == type)
        {
            code = code.Right(code.GetLength() - type.GetLength());
            return;
        }
    }
}

CString COptionsDlg::GetCodeType(const CString & code)
{
    for (const auto& type : StockTypeSet)
    {
        if (code.Left(type.GetLength()) == type)
        {
            return type;
        }
    }
    return CString();
}

// 自动识别股票类型
// 返回值: 0=深证, 1=港股, 2=北交所, 3=上证, 4=美股, 5=OKX虚拟货币, 6=其他
int COptionsDlg::AutoDetectStockType(const CString& code)
{
    if (code.IsEmpty())
        return 6; // 其他

    // 先移除可能存在的前缀
    CString pureCode = code;
    RemoveTypeFromCode(pureCode);

    // 检查是否是OKX虚拟货币格式 (如 BTC-USDT, ETH-USDT)
    if (pureCode.Find(_T('-')) != -1)
    {
        CString upper = pureCode;
        upper.MakeUpper();
        if (upper.Find(_T("USDT")) != -1 || upper.Find(_T("USD")) != -1 ||
            upper.Find(_T("BTC")) != -1 || upper.Find(_T("ETH")) != -1)
        {
            return 5; // OKX虚拟货币
        }
    }

    // 检查是否全是数字
    bool allDigits = true;
    for (int i = 0; i < pureCode.GetLength(); i++)
    {
        if (!_istdigit(pureCode[i]))
        {
            allDigits = false;
            break;
        }
    }

    if (allDigits && pureCode.GetLength() == 6)
    {
        // 6位数字代码
        TCHAR firstChar = pureCode[0];

        // 上证: 6开头(A股主板)、5开头(ETF/基金/债券)、9开头(B股)
        if (firstChar == _T('6') || firstChar == _T('5') || firstChar == _T('9'))
            return 3; // 上证

        // 深证: 0开头(主板)、2开头(B股)、3开头(创业板)、1开头(基金/债券)
        if (firstChar == _T('0') || firstChar == _T('2') || firstChar == _T('3') || firstChar == _T('1'))
            return 0; // 深证

        // 北交所: 8、4开头
        if (firstChar == _T('8') || firstChar == _T('4'))
            return 2; // 北交所
    }
    else if (allDigits && pureCode.GetLength() == 5)
    {
        // 5位数字代码 - 港股
        return 1; // 港股
    }
    else if (!allDigits)
    {
        // 包含字母 - 美股
        return 4; // 美股
    }

    return 6; // 其他
}

void COptionsDlg::OnBnClickedOk()
{
    CString code;
    GetDlgItemText(IDC_CODE_EDIT, code);
    if (code.IsEmpty())
    {
        CDialog::OnCancel();
        return;
    }
    CString type = "";
    switch (m_radio_stock_types)
    {
    case 0:
        type = kSZ;
        break;
    case 1:
        type = kHK;
        break;
    case 2:
        type = kBJ;
        break;
    case 3:
        type = kSH;
        break;
    case 4:
        type = kMG;
        break;
    case 5:
        type = kOKX;
        // 将 / 替换为 - (如 BTC/USDT -> BTC-USDT)
        code.Replace(_T('/'), _T('-'));
        // OKX 需要交易对格式，如果用户只输入了币种名称，自动补全 -USDT
        if (code.Find(_T('-')) == -1)
        {
            code += _T("-USDT");
        }
        break;
    }
    RemoveTypeFromCode(code);
    m_stock_code = type + code;

    // 获取别名
    GetDlgItemText(IDC_ALIAS_EDIT, m_stock_alias);

    CDialog::OnOK();
}


void COptionsDlg::OnBnClickedCancel()
{
    CDialog::OnCancel();
}



void COptionsDlg::OnRadioClickedStockTypes()
{
    UpdateData(TRUE);
    TRACE(L"OnRadioClickedStockTypes: %d\n", m_radio_stock_types);
}
