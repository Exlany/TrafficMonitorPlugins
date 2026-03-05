#include "pch.h"
#include "DataManager.h"
#include "Common.h"
#include "Stock.h"
#include <vector>
#include <sstream>
#include "../utilities/IniHelper.h"
#include <iomanip>
#include <random>

// 新架构头文件
#include "Infrastructure/CIniConfigStore.h"
#include "Infrastructure/CWinHttpClient.h"
#include "Infrastructure/CResourceCache.h"
#include "Domain/CStockSettings.h"
#include "Domain/CStockRepository.h"
#include "Domain/CStockCodeResolver.h"

constexpr auto WEB_USERAGENT = _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 Safari/537.36 Edg/135.0.0.0");

CDataManager CDataManager::m_instance;

CDataManager::CDataManager()
{
    // 初始化DPI
    HDC hDC = ::GetDC(HWND_DESKTOP);
    m_dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    ::ReleaseDC(HWND_DESKTOP, hDC);

    // 检测系统默认代码页
    m_system_code_page = GetACP();

    // 初始化新架构
    InitNewArchitecture();
}

CDataManager::~CDataManager()
{
    SaveConfig();

    // 释放图标资源
    std::lock_guard<std::mutex> lock(m_iconsMutex);
    for (auto& pair : m_icons)
    {
        if (pair.second)
        {
            DestroyIcon(pair.second);
        }
    }
    m_icons.clear();
}

void CDataManager::InitNewArchitecture()
{
    // 创建新架构组件
    m_configStore = std::make_unique<StockPlugin::Infrastructure::CIniConfigStore>();
    m_httpClient = std::make_unique<StockPlugin::Infrastructure::CWinHttpClient>();
    m_resourceCache = std::make_unique<StockPlugin::Infrastructure::CResourceCache>();
    m_settings = std::make_unique<StockPlugin::Domain::CStockSettings>();
    m_repository = std::make_unique<StockPlugin::Domain::CStockRepository>();
    m_codeResolver = std::make_unique<StockPlugin::Domain::CStockCodeResolver>();

    // 配置 HTTP 客户端
    m_httpClient->SetSystemCodePage(m_system_code_page);

    // 配置资源缓存
    m_resourceCache->SetDPI(m_dpi);
}

StockPlugin::Domain::CStockSettings& CDataManager::GetSettings()
{
    return *m_settings;
}

StockPlugin::Domain::CStockRepository& CDataManager::GetRepository()
{
    return *m_repository;
}

StockPlugin::Domain::CStockCodeResolver& CDataManager::GetCodeResolver()
{
    return *m_codeResolver;
}

StockPlugin::Infrastructure::CWinHttpClient& CDataManager::GetHttpClient()
{
    return *m_httpClient;
}

StockPlugin::Infrastructure::CResourceCache& CDataManager::GetResourceCache()
{
    return *m_resourceCache;
}

CDataManager &CDataManager::Instance()
{
    return m_instance;
}

void CDataManager::ResetText()
{
    std::lock_guard<std::mutex> lock(GetStockDataMutex());
    m_repository->ClearRealtimeData();
}

static void WritePrivateProfileInt(const wchar_t *app_name, const wchar_t *key_name, int value, const wchar_t *file_path)
{
    wchar_t buff[16];
    swprintf_s(buff, L"%d", value);
    WritePrivateProfileString(app_name, key_name, buff, file_path);
}

void CDataManager::LoadConfig(const std::wstring &config_dir)
{
    // 获取模块的路径
    HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);
    std::wstring module_path = path;
    m_config_path = module_path;
    m_log_path = module_path;
    if (!config_dir.empty())
    {
        size_t index = module_path.find_last_of(L"\\/");
        // 安全处理：如果找不到分隔符，使用整个路径
        std::wstring module_file_name;
        if (index != std::wstring::npos)
        {
            module_file_name = module_path.substr(index + 1);
        }
        else
        {
            module_file_name = module_path;
        }
        size_t dot_index = module_file_name.find_last_of(L".");
        if (dot_index != std::wstring::npos)
        {
            module_file_name = module_file_name.substr(0, dot_index);
        }
        m_config_path = config_dir + module_file_name;
        m_log_path = config_dir + module_file_name;
    }
    m_config_path += L".ini";
    m_log_path += L".log";

    // 配置日志回调
    m_httpClient->SetLogCallback([this](const std::wstring& msg) {
        CCommon::WriteLog(msg.c_str(), m_log_path.c_str());
    });

    // 使用 IniHelper 加载配置到 CStockSettings
    utilities::CIniHelper ini(m_config_path);

    // 创建临时快照用于加载
    SettingsSnapshot snapshot;

    ini.GetStringList(L"config", L"stock_code", snapshot.stockCodes, std::vector<std::wstring>{});
    snapshot.fullDay = ini.GetBool(L"config", L"full_day", true);
    snapshot.showStockName = ini.GetBool(L"config", L"show_stock_name", true);
    snapshot.colorWithPrice = ini.GetBool(L"config", L"color_with_price", true);

    // 配置值范围验证
    int kline_width = ini.GetInt(L"config", L"kline_width", 450);
    snapshot.klineWidth = (kline_width >= 100 && kline_width <= 2000) ? kline_width : 450;

    int kline_height = ini.GetInt(L"config", L"kline_height", 210);
    snapshot.klineHeight = (kline_height >= 50 && kline_height <= 1000) ? kline_height : 210;

    int price_decimal = ini.GetInt(L"config", L"price_decimal", 3);
    snapshot.priceDecimal = (price_decimal >= 2 && price_decimal <= 8) ? price_decimal : 3;

    int display_mode = ini.GetInt(L"config", L"display_mode", 0);
    snapshot.displayMode = (display_mode >= 0 && display_mode <= 3)
        ? static_cast<StockDisplayMode>(display_mode)
        : StockDisplayMode::ShowAll;

    int carousel_interval = ini.GetInt(L"config", L"carousel_interval", 5);
    snapshot.carouselInterval = (carousel_interval >= 1 && carousel_interval <= 60) ? carousel_interval : 5;

    snapshot.checkUpdate = ini.GetBool(L"config", L"check_update", true);

    // 加载自定义别名
    std::vector<std::wstring> alias_list;
    ini.GetStringList(L"config", L"stock_aliases", alias_list, std::vector<std::wstring>{});
    for (const auto& item : alias_list)
    {
        size_t pos = item.find(L'=');
        if (pos != std::wstring::npos)
        {
            std::wstring code = item.substr(0, pos);
            std::wstring alias = item.substr(pos + 1);
            snapshot.aliases[code] = alias;
        }
    }

    // 应用到 CStockSettings
    m_settings->ApplySnapshot(snapshot);
}

void CDataManager::SaveConfig()
{
    if (!m_config_path.empty())
    {
        // 线程安全：获取配置快照
        SettingsSnapshot settings = GetSettingsSnapshot();

        utilities::CIniHelper ini(m_config_path);
        ini.WriteStringList(L"config", L"stock_code", settings.stockCodes);
        ini.WriteBool(L"config", L"full_day", settings.fullDay);
        ini.WriteBool(L"config", L"show_stock_name", settings.showStockName);
        ini.WriteBool(L"config", L"color_with_price", settings.colorWithPrice);
        ini.WriteInt(L"config", L"kline_width", settings.klineWidth);
        ini.WriteInt(L"config", L"kline_height", settings.klineHeight);
        ini.WriteInt(L"config", L"price_decimal", settings.priceDecimal);
        ini.WriteInt(L"config", L"display_mode", static_cast<int>(settings.displayMode));
        ini.WriteInt(L"config", L"carousel_interval", settings.carouselInterval);
        ini.WriteBool(L"config", L"check_update", settings.checkUpdate);

        // 保存自定义别名
        std::vector<std::wstring> alias_list;
        for (const auto& pair : settings.aliases)
        {
            if (!pair.second.empty())
            {
                alias_list.push_back(pair.first + L"=" + pair.second);
            }
        }
        ini.WriteStringList(L"config", L"stock_aliases", alias_list);

        ini.Save();
    }
}

const CString &CDataManager::StringRes(UINT id)
{
    std::lock_guard<std::mutex> lock(m_stringTableMutex);
    auto iter = m_string_table.find(id);
    if (iter != m_string_table.end())
    {
        return iter->second;
    }
    else
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        m_string_table[id].LoadString(id);
        return m_string_table[id];
    }
}

SettingsSnapshot CDataManager::GetSettingsSnapshot() const
{
    return m_settings->CreateSnapshot();
}

void CDataManager::UpdateSettings(const SettingsSnapshot& snapshot)
{
    m_settings->ApplySnapshot(snapshot);
}

std::mutex& CDataManager::GetStockDataMutex()
{
    return m_repository->GetMutex();
}

void CDataManager::DPIFromWindow(CWnd *pWnd)
{
    CWindowDC dc(pWnd);
    HDC hDC = dc.GetSafeHdc();
    int dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    m_dpi.store(dpi);
    m_resourceCache->SetDPI(dpi);
}

int CDataManager::DPI(int pixel)
{
    return m_dpi.load() * pixel / 96;
}

float CDataManager::DPIF(float pixel)
{
    return m_dpi.load() * pixel / 96;
}

int CDataManager::RDPI(int pixel)
{
    return pixel * 96 / m_dpi.load();
}

HICON CDataManager::GetIcon(UINT id)
{
    std::lock_guard<std::mutex> lock(m_iconsMutex);
    auto iter = m_icons.find(id);
    if (iter != m_icons.end())
    {
        return iter->second;
    }
    else
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(id), IMAGE_ICON, DPI(16), DPI(16), 0);
        m_icons[id] = hIcon;
        return hIcon;
    }
}

std::shared_ptr<STOCK::StockData> CDataManager::GetStockData(const std::wstring &code)
{
    return m_repository->GetStockData(code);
}

static double generateRandomDouble()
{
    // 使用 thread_local 确保线程安全
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(gen);
}

void CDataManager::RequestRealtimeData()
{
    TRACE(L"RequestRealtimeData...\n");
    // 线程安全：获取配置快照
    SettingsSnapshot settings = GetSettingsSnapshot();
    std::vector<std::wstring> codes = settings.stockCodes;

    // 分离 OKX 虚拟货币代码和普通股票代码
    std::vector<std::wstring> okx_codes;
    std::vector<std::wstring> stock_codes;

    for (const auto& code : codes)
    {
        if (code.find(L"okx_") == 0)
        {
            okx_codes.push_back(code);
        }
        else
        {
            stock_codes.push_back(code);
        }
    }

    // 请求普通股票数据
    if (!stock_codes.empty())
    {
        std::wstring url{L"https://hq.sinajs.cn/?"};
        std::vector<std::wstring> params;
        params.push_back(L"_=" + std::to_wstring(generateRandomDouble()));
        params.push_back(L"list=" + CCommon::vectorJoinString(stock_codes, L","));

        url += CCommon::vectorJoinString(params, L"&");
        CCommon::WriteLog(url.c_str(), m_log_path.c_str());

        StockPlugin::Core::HttpRequestOptions opts;
        opts.userAgent = WEB_USERAGENT;
        opts.headers = L"Referer: https://finance.sina.com.cn";

        std::string Stock_data;
        if (m_httpClient->Get(url, Stock_data, opts))
        {
            m_repository->GetMarket().LoadRealtimeDataByJson(Stock_data);
        }
    }

    // 请求 OKX 虚拟货币数据
    for (const auto& code : okx_codes)
    {
        RequestOKXData(code);
    }
}

void CDataManager::RequestTimelineData(std::wstring stock_id)
{
    TRACE(L"RequestTimelineData...\n");

    std::wstring url{L"https://cn.finance.sina.com.cn/minline/getMinlineData?"};
    std::vector<std::wstring> params;
    params.push_back(L"symbol=" + stock_id);
    params.push_back(L"version=7.11.0");
    params.push_back(L"dpc=1");

    url += CCommon::vectorJoinString(params, L"&");
    CCommon::WriteLog(url.c_str(), m_log_path.c_str());

    StockPlugin::Core::HttpRequestOptions opts;
    opts.userAgent = WEB_USERAGENT;
    opts.headers = L"Referer: https://finance.sina.com.cn/realstock/company/" + stock_id + L"/nc.shtml";

    std::string responseData;
    if (m_httpClient->Get(url, responseData, opts) && !responseData.empty())
    {
        CString strData(responseData.c_str());
        m_repository->GetMarket().LoadTimelineDataByJson(stock_id, &strData);
    }
    else
    {
        m_repository->GetMarket().LoadTimelineDataByJson(stock_id, NULL);
    }

    Stock::Instance().UpdateKLine();
}

void CDataManager::RequestOKXData(const std::wstring& code)
{
    try
    {
        TRACE(L"RequestOKXData: %s\n", code.c_str());

        // 从 okx_BTC-USDT 提取交易对 BTC-USDT
        if (code.length() <= 4)
        {
            CCommon::WriteLog(L"RequestOKXData: invalid code format", m_log_path.c_str());
            return;
        }
        std::wstring instId = code.substr(4); // 移除 "okx_" 前缀

        // 验证 instId 格式：只允许字母、数字和连字符
        for (wchar_t ch : instId)
        {
            if (!((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z') ||
                  (ch >= L'0' && ch <= L'9') || ch == L'-'))
            {
                CCommon::WriteLog(L"RequestOKXData: invalid instId format", m_log_path.c_str());
                return;
            }
        }

        // OKX API: https://www.okx.com/api/v5/market/ticker?instId=BTC-USDT
        std::wstring url = L"https://www.okx.com/api/v5/market/ticker?instId=" + StockPlugin::Infrastructure::CWinHttpClient::URLEncode(instId);
        CCommon::WriteLog(url.c_str(), m_log_path.c_str());

        StockPlugin::Core::HttpRequestOptions opts;
        opts.userAgent = WEB_USERAGENT;
        opts.utf8 = true;

        std::string okx_data;
        if (m_httpClient->Get(url, okx_data, opts))
        {
            m_repository->GetMarket().LoadOKXDataByJson(code, okx_data);
        }
    }
    catch (...)
    {
        CCommon::WriteLog(L"RequestOKXData failed", m_log_path.c_str());
    }
}
