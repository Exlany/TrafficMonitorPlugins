#include "pch.h"
#include "Stock.h"
#include "DataManager.h"
#include "OptionsDlg.h"
#include "ManagerDialog.h"
#include "Common.h"
#include "StockVersion.h"
#include <cmath>
#include <Shellapi.h>

Stock Stock::m_instance;

Stock::Stock() : m_pFloatingWnd(NULL)
{
    m_items = vector<StockItem>(Stock_ITEM_MAX);
    fill(m_items.begin(), m_items.end(), StockItem());
    for (int index = 0; index < m_items.size(); index++)
    {
        m_items[index].index = index;
    }
}

Stock::~Stock()
{
    DestroyFloatingWnd();
}

Stock &Stock::Instance()
{
    return m_instance;
}

UINT Stock::ThreadCallback(LPVOID dwUser)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CFlagLocker flag_locker(m_instance.m_is_thread_runing);

    if (g_data.m_setting_data.m_stock_codes.empty())
    {
        // CCommon::WriteLog(L"Stock_code not setting!", g_data.m_log_path.c_str());
        g_data.ResetText();
        return 0;
    }

    time_t cur_time = time(nullptr);
    if (cur_time - m_instance.m_last_request_time > 3)
    {
        m_instance.m_last_request_time = cur_time;

        if (g_data.m_setting_data.m_full_day != 1)
        {
            SYSTEMTIME now_time;
            GetLocalTime(&now_time);
            // CCommon::WriteLog(now_time.wHour, g_data.m_log_path.c_str());
            // CCommon::WriteLog(now_time.wMinute, g_data.m_log_path.c_str());
            if (now_time.wHour < 9 || now_time.wHour > 15 || (now_time.wHour == 15 && now_time.wMinute > 30))
            {
                CCommon::WriteLog(L"Not currently in trading time!", g_data.m_log_path.c_str());
                g_data.ResetText();
                return 0;
            }
        }

        // 禁用选项设置中的“更新”按钮
        m_instance.DisableUpdateCommand();

        g_data.RequestRealtimeData();

        // 启用选项设置中的“更新”按钮
        m_instance.EnableUpdateCommand();
    }
    return 0;
}

void Stock::LoadContextMenu()
{
    if (m_menu.m_hMenu == NULL)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        m_menu.LoadMenu(IDR_MENU1);
    }
}

IPluginItem *Stock::GetItem(int index)
{
    auto& codes = g_data.m_setting_data.m_stock_codes;
    size_t codes_size = codes.size();
    if (codes_size == 0)
    {
        if (index > 0)
            return nullptr;
        return &(m_items[0]);
    }

    // 2个以上股票时，只返回一个Item，在DrawItem中绘制2xN矩阵
    if (codes_size >= 2)
    {
        if (index > 0)
            return nullptr;
        return &(m_items[0]);
    }

    // 单个股票
    if (index > 0)
        return nullptr;
    return &(m_items[0]);
}

const wchar_t *Stock::GetTooltipInfo()
{
    return L"";
}

void Stock::DataRequired()
{
    auto& codes = g_data.m_setting_data.m_stock_codes;

    // 根据显示模式更新 m_items[0].stock_id
    if (!codes.empty())
    {
        switch (g_data.m_setting_data.m_display_mode)
        {
        case StockDisplayMode::Carousel:
            // 轮播模式：检查是否需要切换
            if (codes.size() > 1)
            {
                time_t now = time(nullptr);
                if (now - m_last_carousel_time >= g_data.m_setting_data.m_carousel_interval)
                {
                    m_last_carousel_time = now;
                    m_current_display_index = (m_current_display_index + 1) % codes.size();
                }
            }
            if (m_current_display_index >= codes.size())
                m_current_display_index = 0;
            m_items[0].stock_id = codes[m_current_display_index];
            break;

        case StockDisplayMode::Manual:
            // 手动模式：使用当前索引
            if (m_current_display_index >= codes.size())
                m_current_display_index = 0;
            m_items[0].stock_id = codes[m_current_display_index];
            break;

        case StockDisplayMode::Smart:
            // 智能模式：使用涨跌幅最大的股票
            m_items[0].stock_id = codes[GetSmartIndex()];
            break;

        default:
            // ShowAll 模式不需要特殊处理
            break;
        }
    }

    static time_t last_req_time{-1};
    time_t cur_time = time(nullptr);
    if (cur_time - m_instance.m_last_request_time > 3)
    {
        last_req_time = cur_time;
        SendStockInfoRequest();
    }
    std::lock_guard<std::mutex> lock(m_wndMutex);
    if (m_pFloatingWnd != NULL && ::IsWindow(m_pFloatingWnd->GetSafeHwnd()))
    {
        m_pFloatingWnd->SendMessage(FWND_MSG_REQUEST_DATA, cur_time, 0);
        // DWORD_PTR dwResult = 0;
        // LRESULT lr = ::SendMessageTimeout(
        //     m_pFloatingWnd->GetSafeHwnd(),  // 目标窗口句柄
        //     FWND_MSG_REQUEST_DATA,          // 消息ID
        //     cur_time,                       // wParam
        //     0,                              // lParam
        //     SMTO_ABORTIFHUNG | SMTO_BLOCK,  // 如果窗口挂起则放弃，并阻塞调用线程
        //     2000,                           // 2秒超时
        //     &dwResult);                     // 接收返回值

        // if (lr == 0) // 失败
        //{
        //     DWORD dwErr = GetLastError();
        //     // 处理错误：记录日志或销毁无效窗口等
        //     if (dwErr == ERROR_TIMEOUT)
        //     {
        //         TRACE("SendMessageTimeout timed out\n");
        //     }
        // }
    }
}

ITMPlugin::OptionReturn Stock::ShowOptionsDialog(void *hParent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CWnd *pParent = CWnd::FromHandle((HWND)hParent);
    if (ShowStockManageDlg(pParent) == IDOK)
    {
        return ITMPlugin::OR_OPTION_CHANGED;
    }
    return ITMPlugin::OR_OPTION_UNCHANGED;
}

const wchar_t *Stock::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:
        return g_data.StringRes(IDS_PLUGIN_NAME).GetString();
    case TMI_DESCRIPTION:
        return g_data.StringRes(IDS_PLUGIN_DESCRIPTION).GetString();
    case TMI_AUTHOR:
        return L"CListery";
    case TMI_COPYRIGHT:
        return L"Copyright (C) by CListery 2022";
    case ITMPlugin::TMI_URL:
        return L"https://github.com/zhongyang219/TrafficMonitorPlugins";
    case TMI_VERSION:
        return STOCK_VERSION_STR;
    default:
        break;
    }
    return L"";
}

void Stock::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data)
{
    switch (index)
    {
    case ITMPlugin::EI_CONFIG_DIR:
        // 从配置文件读取配置
        g_data.LoadConfig(std::wstring(data));
        updateItems();
        // 启动后台线程检查更新
        if (g_data.m_setting_data.m_check_update)
        {
            AfxBeginThread(CheckUpdateThread, nullptr);
        }
        break;
    case ITMPlugin::EI_TASKBAR_WND_VALUE_RIGHT_ALIGN:
        // 获取TrafficMonitor任务栏窗口中"数值右对齐"设置
        g_data.m_right_align = (_wtoi(data) != 0);
        break;
    default:
        break;
    }
}

int Stock::GetCommandCount()
{
    return 1;
}

const wchar_t *Stock::GetCommandName(int command_index)
{
    switch (command_index)
    {
    case 0:
        return g_data.StringRes(IDS_MENU_UPDATE_STOCK).GetString();
    }
    return nullptr;
}

void Stock::OnPluginCommand(int command_index, void *hWnd, void *para)
{
    switch (command_index)
    {
    case 0:
        SendStockInfoRequest();
        break;
    }
}

void *Stock::GetPluginIcon()
{
    return g_data.GetIcon(IDI_STOCK);
}

void Stock::updateItems()
{
    for (size_t i = 0; i < m_items.size(); i++)
    {
        m_items[i].enable = FALSE;
    }
    for (size_t index = 0; index < g_data.m_setting_data.m_stock_codes.size(); index++)
    {
        std::wstring key = g_data.m_setting_data.m_stock_codes[index];
        if (index > m_items.size() - 1)
        {
            break;
        }
        m_items[index].enable = TRUE;
        m_items[index].stock_id = key;
    }
}

INT_PTR Stock::ShowStockManageDlg(CWnd *pWnd)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CManagerDialog dlg(pWnd);
    dlg.m_data = g_data.m_setting_data;
    m_option_dlg = &dlg;
    INT_PTR rtn = dlg.DoModal();
    m_option_dlg = nullptr;
    if (rtn == IDOK)
    {
        g_data.m_setting_data = dlg.m_data;
        updateItems();
        g_data.SaveConfig();
    }
    return rtn;
}

void Stock::SendStockInfoRequest()
{
    if (!m_is_thread_runing) // 确保线程已退出
        AfxBeginThread(ThreadCallback, nullptr);
}

void Stock::ShowContextMenu(CWnd *pWnd)
{
    LoadContextMenu();
    CMenu *context_menu = m_menu.GetSubMenu(0);
    if (context_menu != nullptr)
    {
        CPoint point1;
        GetCursorPos(&point1);
        DWORD id = context_menu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point1.x, point1.y, pWnd);
        // 点击了“管理”
        if (id == ID_OPTIONS)
        {
            ShowStockManageDlg(pWnd);
        }
        // 点击了“更新”
        else if (id == ID_UPDATE)
        {
            SendStockInfoRequest();
        }
    }
}

void Stock::ShowFloatingWnd(void *hWnd, CPoint ptScreen, std::wstring stock_id)
{
    // 如果已有悬浮窗，先销毁
    DestroyFloatingWnd();

    ClientToScreen((HWND)hWnd, &ptScreen);

    CWnd *pWnd = CWnd::FromHandle((HWND)hWnd);

    CFont *font = pWnd->GetParent()->GetFont();

    std::lock_guard<std::mutex> lock(m_wndMutex);
    // 创建新的悬浮窗
    m_pFloatingWnd = new CFloatingWnd;
    if (!m_pFloatingWnd->Create(font, ptScreen, stock_id))
    {
        delete m_pFloatingWnd;
        m_pFloatingWnd = NULL;
    }
}

void Stock::DestroyFloatingWnd()
{
    std::lock_guard<std::mutex> lock(m_wndMutex);
    if (m_pFloatingWnd != NULL && ::IsWindow(m_pFloatingWnd->GetSafeHwnd()))
    {
        m_pFloatingWnd->DestroyWindow();
        delete m_pFloatingWnd;
        m_pFloatingWnd = NULL;
    }
}

void Stock::UpdateKLine()
{
    std::lock_guard<std::mutex> lock(m_wndMutex);
    if (m_pFloatingWnd != NULL && ::IsWindow(m_pFloatingWnd->GetSafeHwnd()))
    {
        m_pFloatingWnd->SendMessage(FWND_MSG_UPDATE_STATUS, FALSE, 0);
        // DWORD_PTR dwResult = 0;
        // LRESULT lr = ::SendMessageTimeout(
        //     m_pFloatingWnd->GetSafeHwnd(),  // 目标窗口句柄
        //     FWND_MSG_UPDATE_STATUS,          // 消息ID
        //     FALSE,                       // wParam
        //     0,                              // lParam
        //     SMTO_ABORTIFHUNG | SMTO_BLOCK,  // 如果窗口挂起则放弃，并阻塞调用线程
        //     2000,                           // 2秒超时
        //     &dwResult);                     // 接收返回值

        // if (lr == 0) // 失败
        //{
        //     DWORD dwErr = GetLastError();
        //     // 处理错误：记录日志或销毁无效窗口等
        //     if (dwErr == ERROR_TIMEOUT)
        //     {
        //         TRACE("SendMessageTimeout timed out\n");
        //     }
        // }
    }
}

void Stock::SwitchToNextStock()
{
    auto& codes = g_data.m_setting_data.m_stock_codes;
    if (codes.size() > 1)
    {
        m_current_display_index = (m_current_display_index + 1) % codes.size();
        // 立即更新 m_items[0].stock_id
        m_items[0].stock_id = codes[m_current_display_index];
    }
}

size_t Stock::GetSecondRowIndex()
{
    auto& codes = g_data.m_setting_data.m_stock_codes;
    if (codes.size() <= 1)
        return 0;

    // 第二行显示当前索引的下一个
    size_t firstIdx = m_current_display_index;
    size_t secondIdx = (firstIdx + 1) % codes.size();
    return secondIdx;
}

void Stock::DisableUpdateCommand()
{
    // if (m_option_dlg != nullptr)
    //     m_option_dlg->EnableUpdateBtn(false);
    if (m_menu.m_hMenu != NULL)
        m_menu.EnableMenuItem(ID_UPDATE, MF_BYCOMMAND | MF_GRAYED);
}

void Stock::EnableUpdateCommand()
{
    // if (m_instance.m_option_dlg != nullptr)
    //     m_instance.m_option_dlg->EnableUpdateBtn(true);
    if (m_menu.m_hMenu != NULL)
        m_menu.EnableMenuItem(ID_UPDATE, MF_BYCOMMAND | MF_ENABLED);
}

ITMPlugin *TMPluginGetInstance()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return &Stock::Instance();
}

int Stock::GetSmartIndex()
{
    auto& codes = g_data.m_setting_data.m_stock_codes;
    if (codes.empty())
        return 0;

    int maxIndex = 0;
    double maxScore = 0;

    // 注意：此函数被 GetItem() 调用，不应在此获取 m_stockDataMutex 锁
    // 因为调用方可能已经持有锁，会导致死锁
    for (size_t i = 0; i < codes.size() && i < m_items.size(); i++)
    {
        auto data = g_data.GetStockData(codes[i]);
        if (data && data->realTimeData.prevClosePrice > 0)
        {
            // 计算当日涨跌幅绝对值
            double dailyChange = abs(data->realTimeData.currentPrice - data->realTimeData.prevClosePrice);
            double dailyChangePercent = dailyChange / data->realTimeData.prevClosePrice;

            // 计算10分钟内的涨跌幅（通过分时数据）
            double recentChangePercent = 0;
            auto timelineData = data->getTimelineData();
            if (timelineData && timelineData->data.size() > 1)
            {
                // 获取最近10分钟的数据点（假设每分钟一个点）
                size_t dataSize = timelineData->data.size();
                size_t startIdx = (dataSize > 10) ? (dataSize - 10) : 0;

                STOCK::Price startPrice = timelineData->data[startIdx].price;
                STOCK::Price endPrice = timelineData->data[dataSize - 1].price;

                if (startPrice > 0)
                {
                    recentChangePercent = abs(endPrice - startPrice) / startPrice;
                }
            }

            // 综合评分：当日涨跌幅(权重0.4) + 10分钟涨跌幅(权重0.6)
            // 10分钟变化更能反映当前活跃度
            double score = dailyChangePercent * 0.4 + recentChangePercent * 0.6;

            if (score > maxScore)
            {
                maxScore = score;
                maxIndex = static_cast<int>(i);
            }
        }
    }
    return maxIndex;
}

UINT Stock::CheckUpdateThread(LPVOID pParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // 等待网络连接就绪，最多等待60秒
    // 每5秒检查一次网络连接
    int maxRetries = 12;
    bool networkReady = false;

    for (int i = 0; i < maxRetries; i++)
    {
        // 延迟5秒
        Sleep(5000);

        // 检查网络连接
        if (CCommon::IsNetworkAvailable())
        {
            networkReady = true;
            break;
        }
    }

    if (!networkReady)
    {
        // 网络不可用，放弃检查更新
        return 0;
    }

    // 获取当前版本号
    const wchar_t* currentVersion = m_instance.GetInfo(TMI_VERSION);
    if (!currentVersion)
        return 0;

    CCommon::UpdateInfo info;
    if (CCommon::CheckForUpdate(currentVersion, info))
    {
        // 有新版本
        CString msg;
        if (!info.download_url.empty())
        {
            // 有下载地址，询问是否自动更新
            msg.Format(_T("Stock插件发现新版本 %s\n当前版本: %s\n\n是否自动下载并更新？\n（更新后将自动重启TrafficMonitor）"),
                info.version.c_str(), currentVersion);

            int result = AfxMessageBox(msg, MB_YESNOCANCEL | MB_ICONINFORMATION);
            if (result == IDYES)
            {
                // 自动更新
                std::wstring pluginDir = CCommon::GetPluginDirectory();

                if (CCommon::PerformUpdate(info.download_url, pluginDir))
                {
                    // 更新成功，重启 TrafficMonitor
                    if (AfxMessageBox(_T("更新下载完成！\n是否立即重启TrafficMonitor以应用更新？"), MB_YESNO | MB_ICONQUESTION) == IDYES)
                    {
                        CCommon::RestartTrafficMonitor();
                    }
                }
                else
                {
                    // 自动更新失败，提示手动下载
                    msg.Format(_T("自动更新失败，是否前往下载页面手动下载？"));
                    if (AfxMessageBox(msg, MB_YESNO | MB_ICONWARNING) == IDYES)
                    {
                        ShellExecute(NULL, _T("open"), info.release_page_url.c_str(), NULL, NULL, SW_SHOW);
                    }
                }
            }
            else if (result == IDNO)
            {
                // 打开下载页面手动下载
                ShellExecute(NULL, _T("open"), info.release_page_url.c_str(), NULL, NULL, SW_SHOW);
            }
            // IDCANCEL: 不做任何操作
        }
        else
        {
            // 没有下载地址，只能手动下载
            msg.Format(_T("Stock插件发现新版本 %s\n当前版本: %s\n\n是否前往下载页面？"),
                info.version.c_str(), currentVersion);

            if (AfxMessageBox(msg, MB_YESNO | MB_ICONINFORMATION) == IDYES)
            {
                ShellExecute(NULL, _T("open"), info.release_page_url.c_str(), NULL, NULL, SW_SHOW);
            }
        }
    }

    return 0;
}
