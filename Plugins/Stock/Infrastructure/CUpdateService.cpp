#include "pch.h"
#include "CUpdateService.h"
#include "../Utils/StringUtils.h"
#include <afxinet.h>
#include <shldisp.h>
#include <comdef.h>
#include <Shellapi.h>
#include "utilities/yyjson/yyjson.h"

namespace StockPlugin {
namespace Infrastructure {

CUpdateService::CUpdateService() = default;

CUpdateService::~CUpdateService() = default;

void CUpdateService::SetLogCallback(std::function<void(const std::wstring&)> callback)
{
    m_logCallback = std::move(callback);
}

void CUpdateService::Log(const std::wstring& message)
{
    if (m_logCallback)
    {
        m_logCallback(message);
    }
}

int CUpdateService::CompareVersion(const std::wstring& v1, const std::wstring& v2)
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

bool CUpdateService::CheckForUpdate(const std::wstring& currentVersion, UpdateInfo& info)
{
    try
    {
        // GitHub API 获取 releases 列表
        std::wstring url = L"https://api.github.com/repos/Exlany/TrafficMonitorPlugins/releases";

        auto sessionDeleter = [](CInternetSession* s) { if (s) { s->Close(); delete s; } };
        auto fileDeleter = [](CHttpFile* f) { if (f) { f->Close(); delete f; } };

        std::unique_ptr<CInternetSession, decltype(sessionDeleter)> session(
            new CInternetSession(L"Stock Plugin Update Checker"), sessionDeleter);
        session->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 10000);
        session->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 10000);

        std::unique_ptr<CHttpFile, decltype(fileDeleter)> pFile(
            (CHttpFile*)session->OpenURL(url.c_str(), 1,
                INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE,
                L"Accept: application/vnd.github.v3+json", 0),
            fileDeleter);

        if (!pFile)
            return false;

        DWORD dwStatusCode;
        pFile->QueryInfoStatusCode(dwStatusCode);

        if (dwStatusCode != HTTP_STATUS_OK)
            return false;

        // 读取响应
        std::string response;
        char buffer[4096];
        UINT nRead;
        while ((nRead = pFile->Read(buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[nRead] = '\0';
            response += buffer;
        }

        // 使用 yyjson 解析
        auto docDeleter = [](yyjson_doc* d) { if (d) yyjson_doc_free(d); };
        std::unique_ptr<yyjson_doc, decltype(docDeleter)> doc(
            yyjson_read(response.c_str(), response.size(), 0), docDeleter);
        if (!doc)
            return false;

        yyjson_val* root = yyjson_doc_get_root(doc.get());
        if (!root || !yyjson_is_arr(root))
            return false;

        std::string latestStockTag;
        std::string latestStockVersion;
        std::string downloadUrl;
        std::string releasePageUrl;

        yyjson_val* release;
        yyjson_arr_iter iter;
        yyjson_arr_iter_init(root, &iter);
        while ((release = yyjson_arr_iter_next(&iter)))
        {
            yyjson_val* tagVal = yyjson_obj_get(release, "tag_name");
            if (!tagVal) continue;
            const char* tagName = yyjson_get_str(tagVal);
            if (!tagName || strncmp(tagName, "Stock_V", 7) != 0)
                continue;

            latestStockTag = tagName;
            latestStockVersion = tagName + 7;

            yyjson_val* htmlUrlVal = yyjson_obj_get(release, "html_url");
            if (htmlUrlVal)
                releasePageUrl = yyjson_get_str(htmlUrlVal);

            // 遍历 assets 查找下载 URL
            yyjson_val* assets = yyjson_obj_get(release, "assets");
            if (assets && yyjson_is_arr(assets))
            {
                yyjson_val* asset;
                yyjson_arr_iter asset_iter;
                yyjson_arr_iter_init(assets, &asset_iter);
                while ((asset = yyjson_arr_iter_next(&asset_iter)))
                {
                    yyjson_val* urlVal = yyjson_obj_get(asset, "browser_download_url");
                    if (!urlVal) continue;
                    const char* assetUrlStr = yyjson_get_str(urlVal);
                    if (!assetUrlStr) continue;

                    std::string assetUrl = assetUrlStr;
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
            }

            break;
        }

        if (latestStockVersion.empty())
            return false;

        info.version = Utils::StringUtils::ToUnicode(latestStockVersion, true);
        info.downloadUrl = Utils::StringUtils::ToUnicode(downloadUrl, true);
        info.releasePageUrl = Utils::StringUtils::ToUnicode(releasePageUrl, true);

        if (info.releasePageUrl.empty())
        {
            std::string defaultUrl = "https://github.com/Exlany/TrafficMonitorPlugins/releases/tag/" + latestStockTag;
            info.releasePageUrl = Utils::StringUtils::ToUnicode(defaultUrl, true);
        }

        return CompareVersion(currentVersion, info.version) < 0;
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

bool CUpdateService::DownloadFile(const std::wstring& url, const std::wstring& savePath)
{
    try
    {
        auto sessionDeleter = [](CInternetSession* s) { if (s) { s->Close(); delete s; } };
        auto fileDeleter = [](CHttpFile* f) { if (f) { f->Close(); delete f; } };

        std::unique_ptr<CInternetSession, decltype(sessionDeleter)> session(
            new CInternetSession(L"Stock Plugin Updater"), sessionDeleter);
        session->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 30000);
        session->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 30000);

        std::unique_ptr<CHttpFile, decltype(fileDeleter)> pFile(
            (CHttpFile*)session->OpenURL(url.c_str(), 1,
                INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE,
                NULL, 0),
            fileDeleter);

        if (!pFile)
            return false;

        DWORD dwStatusCode;
        pFile->QueryInfoStatusCode(dwStatusCode);

        if (dwStatusCode != HTTP_STATUS_OK)
            return false;

        CFile localFile;
        if (!localFile.Open(savePath.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
            return false;

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

bool CUpdateService::UnzipFile(const std::wstring& zipPath, const std::wstring& destDir)
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

                    // 等待解压完成（轮询检查而非固定等待）
                    if (success)
                    {
                        for (int i = 0; i < 30; ++i)  // 最多等待 3 秒
                        {
                            Sleep(100);
                            // Shell 解压是异步的，简单等待一段时间
                        }
                    }
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

bool CUpdateService::PerformUpdate(const std::wstring& zipUrl, const std::wstring& pluginDir)
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

    // 清理临时文件（忽略删除失败，可能目录非空）
    DeleteFileW(zipPath.c_str());
    DeleteFileW(newDllPath.c_str());
    // 注意：RemoveDirectory 只能删除空目录，非空目录会失败
    // 这里不检查返回值，因为临时文件清理失败不影响更新结果
    RemoveDirectoryW(extractDir.c_str());
    RemoveDirectoryW(tempDir.c_str());

    return true;
}

std::wstring CUpdateService::GetPluginDirectory()
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

void CUpdateService::RestartTrafficMonitor()
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

} // namespace Infrastructure
} // namespace StockPlugin
