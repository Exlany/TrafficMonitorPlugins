#include "pch.h"
#include "CUpdateChecker.h"
#include "../Infrastructure/CWinHttpClient.h"
#include <shldisp.h>
#include <comdef.h>
#include <vector>

namespace StockPlugin {
namespace Application {

CUpdateChecker::CUpdateChecker() = default;

CUpdateChecker::~CUpdateChecker() = default;

void CUpdateChecker::SetHttpClient(std::shared_ptr<Core::IHttpClient> client)
{
    m_httpClient = std::move(client);
}

void CUpdateChecker::SetLogCallback(std::function<void(const std::wstring&)> callback)
{
    m_logCallback = std::move(callback);
}

void CUpdateChecker::Log(const std::wstring& message)
{
    if (m_logCallback)
    {
        m_logCallback(message);
    }
}

int CUpdateChecker::CompareVersion(const std::wstring& v1, const std::wstring& v2)
{
    std::vector<int> ver1, ver2;

    // 解析 v1
    std::wstring num;
    for (size_t i = 0; i <= v1.length(); i++)
    {
        if (i == v1.length() || v1[i] == L'.')
        {
            if (!num.empty())
            {
                ver1.push_back(_wtoi(num.c_str()));
                num.clear();
            }
        }
        else if (iswdigit(v1[i]))
        {
            num += v1[i];
        }
    }

    // 解析 v2
    num.clear();
    for (size_t i = 0; i <= v2.length(); i++)
    {
        if (i == v2.length() || v2[i] == L'.')
        {
            if (!num.empty())
            {
                ver2.push_back(_wtoi(num.c_str()));
                num.clear();
            }
        }
        else if (iswdigit(v2[i]))
        {
            num += v2[i];
        }
    }

    // 补齐长度
    while (ver1.size() < ver2.size()) ver1.push_back(0);
    while (ver2.size() < ver1.size()) ver2.push_back(0);

    // 逐位比较
    for (size_t i = 0; i < ver1.size(); i++)
    {
        if (ver1[i] < ver2[i]) return -1;
        if (ver1[i] > ver2[i]) return 1;
    }
    return 0;
}

bool CUpdateChecker::CheckForUpdate(const std::wstring& currentVersion, UpdateInfo& info)
{
    if (!m_httpClient)
        return false;

    Core::HttpRequestOptions options;
    options.userAgent = L"Stock Plugin Update Checker";
    options.headers = L"Accept: application/vnd.github.v3+json";
    options.connectTimeout = 10000;
    options.receiveTimeout = 10000;

    std::string response;
    if (!m_httpClient->Get(L"https://api.github.com/repos/Exlany/TrafficMonitorPlugins/releases", response, options))
        return false;

    // 查找最新的 Stock_V* release
    std::string latestStockTag;
    std::string latestStockVersion;
    std::string downloadUrl;
    std::string releasePageUrl;
    size_t searchPos = 0;

    while (true)
    {
        size_t tagPos = response.find("\"tag_name\"", searchPos);
        if (tagPos == std::string::npos)
            break;

        size_t colonPos = response.find(':', tagPos);
        size_t quoteStart = response.find('"', colonPos + 1);
        size_t quoteEnd = response.find('"', quoteStart + 1);

        if (quoteStart == std::string::npos || quoteEnd == std::string::npos)
            break;

        std::string tagName = response.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        searchPos = quoteEnd + 1;

        if (tagName.find("Stock_V") == 0)
        {
            std::string version = tagName.substr(7);
            latestStockTag = tagName;
            latestStockVersion = version;

            // 查找 html_url
            size_t htmlUrlPos = response.find("\"html_url\"", searchPos);
            if (htmlUrlPos != std::string::npos && htmlUrlPos < response.find("\"tag_name\"", searchPos))
            {
                size_t urlColonPos = response.find(':', htmlUrlPos);
                size_t urlQuoteStart = response.find('"', urlColonPos + 1);
                size_t urlQuoteEnd = response.find('"', urlQuoteStart + 1);
                if (urlQuoteStart != std::string::npos && urlQuoteEnd != std::string::npos)
                {
                    releasePageUrl = response.substr(urlQuoteStart + 1, urlQuoteEnd - urlQuoteStart - 1);
                }
            }

            // 查找 assets 中的 zip 下载地址
            size_t assetsPos = response.find("\"assets\"", searchPos);
            if (assetsPos != std::string::npos)
            {
                size_t nextReleasePos = response.find("\"tag_name\"", searchPos);
                size_t assetsEndPos = (nextReleasePos != std::string::npos) ? nextReleasePos : response.length();

                size_t downloadUrlPos = assetsPos;
                while (downloadUrlPos < assetsEndPos)
                {
                    downloadUrlPos = response.find("\"browser_download_url\"", downloadUrlPos);
                    if (downloadUrlPos == std::string::npos || downloadUrlPos >= assetsEndPos)
                        break;

                    size_t dlColonPos = response.find(':', downloadUrlPos);
                    size_t dlQuoteStart = response.find('"', dlColonPos + 1);
                    size_t dlQuoteEnd = response.find('"', dlQuoteStart + 1);

                    if (dlQuoteStart != std::string::npos && dlQuoteEnd != std::string::npos)
                    {
                        std::string assetUrl = response.substr(dlQuoteStart + 1, dlQuoteEnd - dlQuoteStart - 1);
                        if (assetUrl.find("Stock") != std::string::npos &&
                            (assetUrl.find(".zip") != std::string::npos || assetUrl.find(".ZIP") != std::string::npos))
                        {
                            #if defined(_M_X64) || defined(__x86_64__)
                                if (assetUrl.find("x64") != std::string::npos)
                                {
                                    downloadUrl = assetUrl;
                                    break;
                                }
                                else if (assetUrl.find("all-architectures") != std::string::npos && downloadUrl.empty())
                                {
                                    downloadUrl = assetUrl;
                                }
                            #elif defined(_M_ARM64)
                                if (assetUrl.find("arm64") != std::string::npos)
                                {
                                    downloadUrl = assetUrl;
                                    break;
                                }
                                else if (assetUrl.find("all-architectures") != std::string::npos && downloadUrl.empty())
                                {
                                    downloadUrl = assetUrl;
                                }
                            #else
                                if (assetUrl.find("x86") != std::string::npos || assetUrl.find("Win32") != std::string::npos)
                                {
                                    downloadUrl = assetUrl;
                                    break;
                                }
                                else if (assetUrl.find("all-architectures") != std::string::npos && downloadUrl.empty())
                                {
                                    downloadUrl = assetUrl;
                                }
                            #endif
                        }
                    }
                    downloadUrlPos = dlQuoteEnd + 1;
                }
            }

            break;
        }
    }

    if (latestStockVersion.empty())
        return false;

    info.version = Infrastructure::CWinHttpClient::StrToUnicode(latestStockVersion.c_str(), true);
    info.downloadUrl = Infrastructure::CWinHttpClient::StrToUnicode(downloadUrl.c_str(), true);
    info.releasePageUrl = Infrastructure::CWinHttpClient::StrToUnicode(releasePageUrl.c_str(), true);

    if (info.releasePageUrl.empty())
    {
        std::string defaultUrl = "https://github.com/Exlany/TrafficMonitorPlugins/releases/tag/" + latestStockTag;
        info.releasePageUrl = Infrastructure::CWinHttpClient::StrToUnicode(defaultUrl.c_str(), true);
    }

    return CompareVersion(currentVersion, info.version) < 0;
}

bool CUpdateChecker::IsNetworkAvailable()
{
    if (m_httpClient)
    {
        return m_httpClient->IsNetworkAvailable();
    }
    return false;
}

std::wstring CUpdateChecker::GetPluginDirectory()
{
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)GetPluginDirectory, &hModule);

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);

    std::wstring fullPath = path;
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
    {
        return fullPath.substr(0, lastSlash);
    }
    return fullPath;
}

void CUpdateChecker::RestartTrafficMonitor()
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    sei.lpFile = exePath;
    sei.nShow = SW_SHOW;

    if (ShellExecuteExW(&sei))
    {
        Sleep(500);
        ExitProcess(0);
    }
}

bool CUpdateChecker::DownloadFile(const std::wstring& url, const std::wstring& savePath)
{
    if (!m_httpClient)
        return false;

    Core::HttpRequestOptions options;
    options.userAgent = L"Stock Plugin Updater";
    options.connectTimeout = 30000;
    options.receiveTimeout = 30000;

    return m_httpClient->Download(url, savePath, options);
}

bool CUpdateChecker::UnzipFile(const std::wstring& zipPath, const std::wstring& destDir)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    bool needUninit = SUCCEEDED(hr);

    bool success = false;

    IShellDispatch* pShellDispatch = NULL;
    hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pShellDispatch);

    if (SUCCEEDED(hr) && pShellDispatch)
    {
        BSTR bstrZipPath = SysAllocString(zipPath.c_str());
        BSTR bstrDestDir = SysAllocString(destDir.c_str());

        Folder* pZipFolder = NULL;
        Folder* pDestFolder = NULL;

        VARIANT vZipPath, vDestDir, vOptions;
        VariantInit(&vZipPath);
        VariantInit(&vDestDir);
        VariantInit(&vOptions);

        vZipPath.vt = VT_BSTR;
        vZipPath.bstrVal = bstrZipPath;

        vDestDir.vt = VT_BSTR;
        vDestDir.bstrVal = bstrDestDir;

        vOptions.vt = VT_I4;
        vOptions.lVal = 4 | 16 | 1024;

        hr = pShellDispatch->NameSpace(vZipPath, &pZipFolder);
        if (SUCCEEDED(hr) && pZipFolder)
        {
            hr = pShellDispatch->NameSpace(vDestDir, &pDestFolder);
            if (SUCCEEDED(hr) && pDestFolder)
            {
                FolderItems* pItems = NULL;
                hr = pZipFolder->Items(&pItems);
                if (SUCCEEDED(hr) && pItems)
                {
                    VARIANT vItems;
                    VariantInit(&vItems);
                    vItems.vt = VT_DISPATCH;
                    vItems.pdispVal = pItems;

                    hr = pDestFolder->CopyHere(vItems, vOptions);
                    success = SUCCEEDED(hr);

                    Sleep(1000);

                    pItems->Release();
                }
                pDestFolder->Release();
            }
            pZipFolder->Release();
        }

        SysFreeString(bstrZipPath);
        SysFreeString(bstrDestDir);
        pShellDispatch->Release();
    }

    if (needUninit)
        CoUninitialize();

    return success;
}

bool CUpdateChecker::PerformUpdate(const std::wstring& zipUrl, const std::wstring& pluginDir)
{
    if (zipUrl.empty())
        return false;

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);

    std::wstring tempDir = tempPath;
    tempDir += L"StockPluginUpdate\\";

    CreateDirectoryW(tempDir.c_str(), NULL);

    std::wstring zipPath = tempDir + L"Stock_update.zip";
    if (!DownloadFile(zipUrl, zipPath))
    {
        Log(L"Failed to download update file");
        return false;
    }

    std::wstring extractDir = tempDir + L"extracted\\";
    CreateDirectoryW(extractDir.c_str(), NULL);

    if (!UnzipFile(zipPath, extractDir))
    {
        Log(L"Failed to extract update file");
        DeleteFileW(zipPath.c_str());
        return false;
    }

    // 查找解压后的 Stock.dll
    std::wstring newDllPath;
    WIN32_FIND_DATAW findData;

    std::wstring searchPath = extractDir + L"Stock.dll";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        newDllPath = extractDir + findData.cFileName;
        FindClose(hFind);
    }
    else
    {
        searchPath = extractDir + L"*";
        hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    wcscmp(findData.cFileName, L".") != 0 &&
                    wcscmp(findData.cFileName, L"..") != 0)
                {
                    std::wstring subDir = extractDir + findData.cFileName + L"\\Stock.dll";
                    if (GetFileAttributesW(subDir.c_str()) != INVALID_FILE_ATTRIBUTES)
                    {
                        newDllPath = subDir;
                        break;
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    if (newDllPath.empty())
    {
        Log(L"Stock.dll not found in update package");
        DeleteFileW(zipPath.c_str());
        return false;
    }

    std::wstring targetDllPath = pluginDir + L"\\Stock.dll";
    std::wstring backupDllPath = pluginDir + L"\\Stock.dll.bak";

    DeleteFileW(backupDllPath.c_str());

    if (!MoveFileExW(targetDllPath.c_str(), backupDllPath.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        MoveFileExW(targetDllPath.c_str(), backupDllPath.c_str(), MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
    }

    if (!CopyFileW(newDllPath.c_str(), targetDllPath.c_str(), FALSE))
    {
        std::wstring pendingDllPath = pluginDir + L"\\Stock.dll.new";
        if (CopyFileW(newDllPath.c_str(), pendingDllPath.c_str(), FALSE))
        {
            MoveFileExW(pendingDllPath.c_str(), targetDllPath.c_str(), MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
        }
        else
        {
            Log(L"Failed to copy new dll");
            MoveFileW(backupDllPath.c_str(), targetDllPath.c_str());
            DeleteFileW(zipPath.c_str());
            return false;
        }
    }

    // 清理临时文件
    DeleteFileW(zipPath.c_str());
    DeleteFileW(newDllPath.c_str());
    RemoveDirectoryW(extractDir.c_str());
    RemoveDirectoryW(tempDir.c_str());

    return true;
}

} // namespace Application
} // namespace StockPlugin
