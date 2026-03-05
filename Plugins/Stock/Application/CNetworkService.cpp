#include "pch.h"
#include "CNetworkService.h"
#include "../Infrastructure/CWinHttpClient.h"
#include "../Domain/CStockRepository.h"
#include "../Utils/StringUtils.h"
#include "../Common.h"
#include <random>

namespace StockPlugin {
namespace Application {

constexpr auto WEB_USERAGENT = _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 Safari/537.36 Edg/135.0.0.0");

CNetworkService::CNetworkService() = default;

CNetworkService::~CNetworkService() = default;

void CNetworkService::SetHttpClient(Infrastructure::CWinHttpClient* client)
{
    m_httpClient = client;
}

void CNetworkService::SetRepository(Domain::CStockRepository* repository)
{
    m_repository = repository;
}

void CNetworkService::SetLogPath(const std::wstring& path)
{
    m_logPath = path;
}

void CNetworkService::SetSystemCodePage(UINT codePage)
{
    m_systemCodePage = codePage;
}

void CNetworkService::Log(const std::wstring& message)
{
    if (!m_logPath.empty())
    {
        CCommon::WriteLog(message.c_str(), m_logPath.c_str());
    }
}

static double generateRandomDouble()
{
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(gen);
}

void CNetworkService::RequestRealtimeData(const std::vector<std::wstring>& stockCodes)
{
    if (!m_httpClient || !m_repository)
        return;

    TRACE(L"CNetworkService::RequestRealtimeData...\n");

    // 分离 OKX 虚拟货币代码和普通股票代码
    std::vector<std::wstring> okxCodes;
    std::vector<std::wstring> normalCodes;

    for (const auto& code : stockCodes)
    {
        if (code.find(L"okx_") == 0)
        {
            okxCodes.push_back(code);
        }
        else
        {
            normalCodes.push_back(code);
        }
    }

    // 请求普通股票数据
    if (!normalCodes.empty())
    {
        std::wstring url{L"https://hq.sinajs.cn/?"};
        std::vector<std::wstring> params;
        params.push_back(L"_=" + std::to_wstring(generateRandomDouble()));
        params.push_back(L"list=" + Utils::StringUtils::Join(normalCodes, L","));

        url += Utils::StringUtils::Join(params, L"&");
        Log(url);

        Core::HttpRequestOptions opts;
        opts.userAgent = WEB_USERAGENT;
        opts.headers = L"Referer: https://finance.sina.com.cn";

        std::string responseData;
        if (m_httpClient->Get(url, responseData, opts))
        {
            m_repository->GetMarket().LoadRealtimeDataByJson(responseData);
        }
    }

    // 请求 OKX 虚拟货币数据
    for (const auto& code : okxCodes)
    {
        RequestOKXData(code);
    }
}

void CNetworkService::RequestTimelineData(const std::wstring& stockId, std::function<void()> callback)
{
    if (!m_httpClient || !m_repository)
        return;

    TRACE(L"CNetworkService::RequestTimelineData...\n");

    std::wstring url{L"https://cn.finance.sina.com.cn/minline/getMinlineData?"};
    std::vector<std::wstring> params;
    params.push_back(L"symbol=" + stockId);
    params.push_back(L"version=7.11.0");
    params.push_back(L"dpc=1");

    url += Utils::StringUtils::Join(params, L"&");
    Log(url);

    Core::HttpRequestOptions opts;
    opts.userAgent = WEB_USERAGENT;
    opts.headers = L"Referer: https://finance.sina.com.cn/realstock/company/" + stockId + L"/nc.shtml";

    std::string responseData;
    if (m_httpClient->Get(url, responseData, opts) && !responseData.empty())
    {
        CString strData(responseData.c_str());
        m_repository->GetMarket().LoadTimelineDataByJson(stockId, &strData);
    }
    else
    {
        m_repository->GetMarket().LoadTimelineDataByJson(stockId, NULL);
    }

    if (callback)
    {
        callback();
    }
}

void CNetworkService::RequestOKXData(const std::wstring& code)
{
    if (!m_httpClient || !m_repository)
        return;

    try
    {
        TRACE(L"CNetworkService::RequestOKXData: %s\n", code.c_str());

        if (code.length() <= 4)
        {
            Log(L"RequestOKXData: invalid code format");
            return;
        }

        std::wstring instId = code.substr(4);

        // 验证 instId 格式
        for (wchar_t ch : instId)
        {
            if (!((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z') ||
                  (ch >= L'0' && ch <= L'9') || ch == L'-'))
            {
                Log(L"RequestOKXData: invalid instId format");
                return;
            }
        }

        std::wstring url = L"https://www.okx.com/api/v5/market/ticker?instId=" + Utils::StringUtils::URLEncode(instId);
        Log(url);

        Core::HttpRequestOptions opts;
        opts.userAgent = WEB_USERAGENT;
        opts.utf8 = true;

        std::string responseData;
        if (m_httpClient->Get(url, responseData, opts))
        {
            m_repository->GetMarket().LoadOKXDataByJson(code, responseData);
        }
    }
    catch (...)
    {
        Log(L"RequestOKXData failed");
    }
}

} // namespace Application
} // namespace StockPlugin
