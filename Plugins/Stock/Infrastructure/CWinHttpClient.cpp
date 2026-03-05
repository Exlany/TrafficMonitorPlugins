#include "pch.h"
#include "CWinHttpClient.h"
#include <afxinet.h>
#include <vector>

namespace StockPlugin {
namespace Infrastructure {

CWinHttpClient::CWinHttpClient() = default;

CWinHttpClient::~CWinHttpClient() = default;

bool CWinHttpClient::Get(const std::wstring& url, std::string& response,
                          const Core::HttpRequestOptions& options)
{
    bool succeed = false;
    try
    {
        auto sessionDeleter = [](CInternetSession* s) {
            if (s) { s->Close(); delete s; }
        };
        auto fileDeleter = [](CHttpFile* f) {
            if (f) { f->Close(); delete f; }
        };

        std::wstring userAgent = options.userAgent.empty() ? L"Stock Plugin" : options.userAgent;

        std::unique_ptr<CInternetSession, decltype(sessionDeleter)> pSession(
            new CInternetSession(userAgent.c_str()), sessionDeleter);

        pSession->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, options.connectTimeout);
        pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, options.receiveTimeout);

        LPCTSTR headers = options.headers.empty() ? nullptr : options.headers.c_str();
        DWORD headersLen = options.headers.empty() ? 0 : static_cast<DWORD>(options.headers.length());

        std::unique_ptr<CHttpFile, decltype(fileDeleter)> pFile(
            (CHttpFile*)pSession->OpenURL(url.c_str(), 1, INTERNET_FLAG_TRANSFER_BINARY, headers, headersLen),
            fileDeleter);

        // 检查 OpenURL 是否成功
        if (!pFile)
        {
            Log(L"OpenURL failed: " + url);
            return false;
        }

        DWORD dwStatusCode;
        pFile->QueryInfoStatusCode(dwStatusCode);

        if (dwStatusCode == HTTP_STATUS_OK)
        {
            // 使用循环读取全部数据（GetLength 可能返回 0）
            std::string rawData;
            char readBuf[4096];
            UINT nRead;
            while ((nRead = pFile->Read(readBuf, sizeof(readBuf))) > 0)
            {
                rawData.append(readBuf, nRead);
            }

            if (options.utf8)
            {
                // 响应本身就是 UTF-8，直接使用
                response = rawData;
            }
            else if (m_systemCodePage == 65001)  // UTF-8 系统
            {
                // 响应是 GBK 编码，需要转换为 UTF-8
                int len = MultiByteToWideChar(936, 0, rawData.c_str(), -1, NULL, 0);
                if (len > 0)
                {
                    std::vector<wchar_t> wBuf(len);
                    MultiByteToWideChar(936, 0, rawData.c_str(), -1, wBuf.data(), len);
                    int len2 = WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, NULL, 0, NULL, NULL);
                    if (len2 > 0)
                    {
                        std::vector<char> finalBuf(len2);
                        WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, finalBuf.data(), len2, NULL, NULL);
                        response = finalBuf.data();
                    }
                }
            }
            else
            {
                response = rawData;
            }
            succeed = true;
        }
    }
    catch (CInternetException* e)
    {
        Log(L"HTTP GET request failed");
        e->Delete();
        succeed = false;
    }
    return succeed;
}

bool CWinHttpClient::Download(const std::wstring& url, const std::wstring& savePath,
                               const Core::HttpRequestOptions& options)
{
    try
    {
        std::wstring userAgent = options.userAgent.empty() ? L"Stock Plugin Updater" : options.userAgent;

        auto sessionDeleter = [](CInternetSession* s) {
            if (s) { s->Close(); delete s; }
        };
        auto fileDeleter = [](CHttpFile* f) {
            if (f) { f->Close(); delete f; }
        };

        std::unique_ptr<CInternetSession, decltype(sessionDeleter)> pSession(
            new CInternetSession(userAgent.c_str()), sessionDeleter);

        pSession->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, options.connectTimeout > 0 ? options.connectTimeout : 30000);
        pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, options.receiveTimeout > 0 ? options.receiveTimeout : 30000);

        std::unique_ptr<CHttpFile, decltype(fileDeleter)> pFile(
            (CHttpFile*)pSession->OpenURL(url.c_str(), 1,
                INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE,
                NULL, 0),
            fileDeleter);

        if (!pFile)
            return false;

        DWORD dwStatusCode;
        pFile->QueryInfoStatusCode(dwStatusCode);

        if (dwStatusCode != HTTP_STATUS_OK)
        {
            return false;
        }

        // 创建本地文件
        CFile localFile;
        if (!localFile.Open(savePath.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
        {
            return false;
        }

        // 下载并写入文件
        char buffer[8192];
        UINT nRead;
        while ((nRead = pFile->Read(buffer, sizeof(buffer))) > 0)
        {
            localFile.Write(buffer, nRead);
        }

        localFile.Close();
        return true;
    }
    catch (CInternetException* e)
    {
        e->Delete();
        return false;
    }
    catch (CFileException* e)
    {
        e->Delete();
        return false;
    }
    catch (...)
    {
        return false;
    }
}

bool CWinHttpClient::IsNetworkAvailable()
{
    try
    {
        auto sessionDeleter = [](CInternetSession* s) {
            if (s) { s->Close(); delete s; }
        };
        auto fileDeleter = [](CHttpFile* f) {
            if (f) { f->Close(); delete f; }
        };

        std::unique_ptr<CInternetSession, decltype(sessionDeleter)> pSession(
            new CInternetSession(L"Network Check", 1, INTERNET_OPEN_TYPE_PRECONFIG), sessionDeleter);

        pSession->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 3000);
        pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 3000);

        std::unique_ptr<CHttpFile, decltype(fileDeleter)> pFile(
            (CHttpFile*)pSession->OpenURL(
                L"https://api.github.com",
                1,
                INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT,
                NULL, 0),
            fileDeleter);

        if (pFile)
        {
            DWORD dwStatusCode = 0;
            pFile->QueryInfoStatusCode(dwStatusCode);
            return (dwStatusCode > 0);
        }

        return false;
    }
    catch (CInternetException* e)
    {
        e->Delete();
        return false;
    }
    catch (...)
    {
        return false;
    }
}

void CWinHttpClient::SetSystemCodePage(UINT codePage)
{
    m_systemCodePage = codePage;
}

void CWinHttpClient::SetLogCallback(std::function<void(const std::wstring&)> callback)
{
    m_logCallback = std::move(callback);
}

// 注意：StrToUnicode, UnicodeToStr, URLEncode 已移至头文件中作为内联函数
// 它们委托给 Utils::StringUtils 实现

void CWinHttpClient::Log(const std::wstring& message)
{
    if (m_logCallback)
    {
        m_logCallback(message);
    }
}

} // namespace Infrastructure
} // namespace StockPlugin
