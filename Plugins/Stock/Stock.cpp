#include "pch.h"
#include "Stock.h"
#include "StockConstants.h"
#include "DataManager.h"
#include "OptionsDlg.h"
#include "ManagerDialog.h"
#include "Common.h"
#include "StockVersion.h"
#include "Infrastructure/CWinHttpClient.h"
#include "Domain/CStockRepository.h"
#include <cmath>
#include <Shellapi.h>
#include <shared_mutex>

using namespace StockConstants;

Stock Stock::m_instance;

Stock::Stock()
{
    m_items = std::vector<StockItem>(MAX_STOCK_ITEMS);
    fill(m_items.begin(), m_items.end(), StockItem());
    for (size_t index = 0; index < m_items.size(); index++)
    {
        m_items[index].index = static_cast<int>(index);
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

    // 使用 RAII 确保线程标志被重置
    struct ThreadGuard {
        std::atomic<bool>& flag;
        ~ThreadGuard() { flag.store(false); }
    } guard{m_instance.m_is_thread_running};

    // 获取设置数据的快照（线程安全）
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();

    if (settings.stockCodes.empty())
    {
        g_data.ResetText();
        return 0;
    }

    time_t cur_time = time(nullptr);
    unsigned __int64 last_req = m_instance.m_last_request_time.load();
    if (cur_time - static_cast<time_t>(last_req) > REQUEST_INTERVAL_SEC)
    {
        m_instance.m_last_request_time.store(static_cast<unsigned __int64>(cur_time));

        if (!settings.fullDay)
        {
            SYSTEMTIME now_time;
            GetLocalTime(&now_time);
            if (now_time.wHour < TRADING_START_HOUR || now_time.wHour > TRADING_END_HOUR || (now_time.wHour == TRADING_END_HOUR && now_time.wMinute > TRADING_START_MINUTE))
            {
                CCommon::WriteLog(L"Not currently in trading time!", g_data.m_log_path.c_str());
                g_data.ResetText();
                return 0;
            }
        }

        // 注意：不在此处禁用/启用菜单项，因为菜单操作应在主线程进行
        // 用户在数据请求期间仍可点击"更新"，但会因 REQUEST_INTERVAL_SEC 限制而被跳过

        g_data.RequestRealtimeData();
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
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    size_t codes_size = settings.stockCodes.size();
    if (codes_size == 0)
    {
        if (index > 0)
            return nullptr;
        std::shared_lock<std::shared_mutex> lock(m_itemsMutex);
        return &(m_items[0]);
    }

    // 2个以上股票时，只返回一个Item，在DrawItem中绘制2xN矩阵
    if (codes_size >= 2)
    {
        if (index > 0)
            return nullptr;
        std::shared_lock<std::shared_mutex> lock(m_itemsMutex);
        return &(m_items[0]);
    }

    // 单个股票
    if (index > 0)
        return nullptr;
    std::shared_lock<std::shared_mutex> lock(m_itemsMutex);
    return &(m_items[0]);
}

const wchar_t *Stock::GetTooltipInfo()
{
    return L"";
}

void Stock::DataRequired()
{
    // 获取设置数据的快照（线程安全）
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;

    // 根据显示模式更新 m_items[0].stock_id
    if (!codes.empty())
    {
        switch (settings.displayMode)
        {
        case StockDisplayMode::Carousel:
            // 轮播模式：检查是否需要切换
            if (codes.size() > 1)
            {
                time_t now = time(nullptr);
                time_t last_carousel = m_last_carousel_time.load();
                if (now - last_carousel >= settings.carouselInterval)
                {
                    m_last_carousel_time.store(now);
                    int cur_idx = m_current_display_index.load();
                    m_current_display_index.store((cur_idx + 1) % static_cast<int>(codes.size()));
                }
            }
            {
                int cur_idx = m_current_display_index.load();
                if (cur_idx >= static_cast<int>(codes.size()))
                {
                    cur_idx = 0;
                    m_current_display_index.store(0);
                }
                std::unique_lock<std::shared_mutex> lock(m_itemsMutex);
                m_items[0].stock_id = codes[cur_idx];
            }
            break;

        case StockDisplayMode::Manual:
            // 手动模式：使用当前索引
            {
                int cur_idx = m_current_display_index.load();
                if (cur_idx >= static_cast<int>(codes.size()))
                {
                    cur_idx = 0;
                    m_current_display_index.store(0);
                }
                std::unique_lock<std::shared_mutex> lock(m_itemsMutex);
                m_items[0].stock_id = codes[cur_idx];
            }
            break;

        case StockDisplayMode::Smart:
            // 智能模式：使用涨跌幅最大的股票
            {
                std::unique_lock<std::shared_mutex> lock(m_itemsMutex);
                m_items[0].stock_id = codes[GetSmartIndex()];
            }
            break;

        default:
            // ShowAll 模式不需要特殊处理
            break;
        }
    }

    time_t cur_time = time(nullptr);
    unsigned __int64 last_req = m_instance.m_last_request_time.load();
    if (cur_time - static_cast<time_t>(last_req) > REQUEST_INTERVAL_SEC)
    {
        SendStockInfoRequest();
    }

    // 使用 PostMessage 减少 TOCTOU 窗口
    {
        std::lock_guard<std::mutex> lock(m_wndMutex);
        if (m_pFloatingWnd)
        {
            HWND hWnd = m_pFloatingWnd->GetSafeHwnd();
            if (hWnd && ::IsWindow(hWnd))
            {
                ::PostMessage(hWnd, FWND_MSG_REQUEST_DATA, static_cast<WPARAM>(cur_time), 0);
            }
        }
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
        {
            SettingsSnapshot settings = g_data.GetSettingsSnapshot();
            if (settings.checkUpdate)
            {
                AfxBeginThread(CheckUpdateThread, nullptr);
            }
        }
        break;
    case ITMPlugin::EI_TASKBAR_WND_VALUE_RIGHT_ALIGN:
        // 获取TrafficMonitor任务栏窗口中"数值右对齐"设置
        g_data.m_right_align.store(_wtoi(data) != 0);
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
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    std::unique_lock<std::shared_mutex> lock(m_itemsMutex);
    for (size_t i = 0; i < m_items.size(); i++)
    {
        m_items[i].enable = FALSE;
    }
    for (size_t index = 0; index < settings.stockCodes.size() && index < m_items.size(); index++)
    {
        std::wstring key = settings.stockCodes[index];
        m_items[index].enable = TRUE;
        m_items[index].stock_id = key;
    }
}

INT_PTR Stock::ShowStockManageDlg(CWnd *pWnd)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CManagerDialog dlg(pWnd);
    dlg.m_data = g_data.GetSettingsSnapshot();
    m_option_dlg = &dlg;
    INT_PTR rtn = dlg.DoModal();
    m_option_dlg = nullptr;
    if (rtn == IDOK)
    {
        g_data.UpdateSettings(dlg.m_data);
        updateItems();
        g_data.SaveConfig();
    }
    return rtn;
}

void Stock::SendStockInfoRequest()
{
    // 使用 compare_exchange 确保线程安全启动
    bool expected = false;
    if (m_is_thread_running.compare_exchange_strong(expected, true))
    {
        CWinThread* pThread = AfxBeginThread(ThreadCallback, nullptr);
        if (pThread == nullptr)
        {
            // 线程创建失败，重置标志
            m_is_thread_running.store(false);
        }
    }
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
    if (!pWnd)
        return;

    CWnd *pParent = pWnd->GetParent();
    CFont *font = pParent ? pParent->GetFont() : nullptr;

    std::lock_guard<std::mutex> lock(m_wndMutex);
    // 创建新的悬浮窗
    m_pFloatingWnd = std::make_unique<CFloatingWnd>();
    if (!m_pFloatingWnd->Create(font, ptScreen, stock_id))
    {
        m_pFloatingWnd.reset();
    }
}

void Stock::DestroyFloatingWnd()
{
    std::lock_guard<std::mutex> lock(m_wndMutex);
    if (m_pFloatingWnd && ::IsWindow(m_pFloatingWnd->GetSafeHwnd()))
    {
        m_pFloatingWnd->DestroyWindow();
    }
    m_pFloatingWnd.reset();
}

void Stock::UpdateKLine()
{
    std::lock_guard<std::mutex> lock(m_wndMutex);
    if (m_pFloatingWnd && ::IsWindow(m_pFloatingWnd->GetSafeHwnd()))
    {
        m_pFloatingWnd->SendMessage(FWND_MSG_UPDATE_STATUS, FALSE, 0);
    }
}

void Stock::SwitchToNextStock()
{
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;
    if (codes.size() > 1)
    {
        int cur_idx = m_current_display_index.load();
        int new_idx = (cur_idx + 1) % static_cast<int>(codes.size());
        m_current_display_index.store(new_idx);
        // 立即更新 m_items[0].stock_id
        std::unique_lock<std::shared_mutex> lock(m_itemsMutex);
        m_items[0].stock_id = codes[new_idx];
    }
}

size_t Stock::GetSecondRowIndex()
{
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;
    if (codes.size() <= 1)
        return 0;

    // 第二行显示当前索引的下一个
    size_t firstIdx = static_cast<size_t>(m_current_display_index.load());
    size_t secondIdx = (firstIdx + 1) % codes.size();
    return secondIdx;
}

void Stock::DisableUpdateCommand()
{
    if (m_menu.m_hMenu != NULL)
        m_menu.EnableMenuItem(ID_UPDATE, MF_BYCOMMAND | MF_GRAYED);
}

void Stock::EnableUpdateCommand()
{
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
    // 获取设置数据的快照（线程安全）
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;
    if (codes.empty())
        return 0;

    int maxIndex = 0;
    double maxScore = 0;

    // 获取数据锁以保护股票数据访问
    std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());
    for (size_t i = 0; i < codes.size() && i < m_items.size(); i++)
    {
        auto data = g_data.GetRepository().GetMarket().getStock(codes[i]);
        if (data && data->realTimeData.prevClosePrice > 0)
        {
            // 计算当日涨跌幅绝对值
            double dailyChange = std::abs(data->realTimeData.currentPrice - data->realTimeData.prevClosePrice);
            double dailyChangePercent = dailyChange / data->realTimeData.prevClosePrice;

            // 计算10分钟内的涨跌幅（通过分时数据）
            double recentChangePercent = 0;
            auto timelineData = data->getTimelineData();
            if (timelineData && timelineData->data.size() > 1)
            {
                // 获取最近10分钟的数据点（假设每分钟一个点）
                size_t dataSize = timelineData->data.size();
                size_t startIdx = (dataSize > RECENT_MINUTES) ? (dataSize - RECENT_MINUTES) : 0;

                STOCK::Price startPrice = timelineData->data[startIdx].price;
                STOCK::Price endPrice = timelineData->data[dataSize - 1].price;

                if (startPrice > 0)
                {
                    recentChangePercent = std::abs(endPrice - startPrice) / startPrice;
                }
            }

            // 综合评分：当日涨跌幅(权重0.4) + 10分钟涨跌幅(权重0.6)
            // 10分钟变化更能反映当前活跃度
            double score = dailyChangePercent * DAILY_CHANGE_WEIGHT + recentChangePercent * RECENT_CHANGE_WEIGHT;

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
    int maxRetries = NETWORK_CHECK_MAX_RETRIES;
    bool networkReady = false;

    for (int i = 0; i < maxRetries; i++)
    {
        // 延迟5秒
        Sleep(NETWORK_CHECK_INTERVAL);

        // 检查网络连接
        if (g_data.GetHttpClient().IsNetworkAvailable())
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
