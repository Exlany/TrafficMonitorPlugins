#include "pch.h"
#include "Common.h"
#include "StockMappings.h"
#include <afxinet.h>
#include <sstream>
#include <memory>
#include <vector>
#include <mutex>
#include "DataManager.h"
#include <shldisp.h>    // Shell API for unzip
#include <comdef.h>
#include "utilities/yyjson/yyjson.h"

std::wstring CCommon::StrToUnicode(const char* str, bool utf8)
{
    if (str == nullptr)
        return std::wstring();
    int size = MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, NULL, 0);
    if (size <= 0) return std::wstring();
    std::vector<wchar_t> buf(size + 1);
    MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, buf.data(), size);
    return std::wstring(buf.data());
}

std::string CCommon::UnicodeToStr(const wchar_t* wstr, bool utf8)
{
    if (wstr == nullptr)
        return std::string();
    int size = WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size <= 0) return std::string();
    std::vector<char> buf(size + 1);
    WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, buf.data(), size, NULL, NULL);
    return std::string(buf.data());
}

bool CCommon::GetURL(const std::wstring& url, std::string& result, bool utf8, LPCTSTR user_agent, LPCTSTR headers, DWORD dwHeadersLength)
{
    bool succeed{ false };
    try
    {
        auto sessionDeleter = [](CInternetSession* s) { if (s) { s->Close(); delete s; } };
        auto fileDeleter = [](CHttpFile* f) { if (f) { f->Close(); delete f; } };

        std::unique_ptr<CInternetSession, decltype(sessionDeleter)> pSession(
            new CInternetSession(user_agent), sessionDeleter);
        std::unique_ptr<CHttpFile, decltype(fileDeleter)> pfile(
            (CHttpFile*)pSession->OpenURL(url.c_str(), 1, INTERNET_FLAG_TRANSFER_BINARY, headers, dwHeadersLength),
            fileDeleter);

        DWORD dwStatusCode;
        pfile->QueryInfoStatusCode(dwStatusCode);
        if (dwStatusCode == HTTP_STATUS_OK)
        {
            DWORD dwTotal = static_cast<DWORD>(pfile->GetLength());
            DWORD dwRead = 0;
            std::vector<char> buffer(dwTotal + 1);
            while (dwRead < dwTotal)
            {
                DWORD dwBytesRead = pfile->Read(buffer.data() + dwRead, dwTotal - dwRead);
                if (dwBytesRead == 0)
                    break;
                dwRead += dwBytesRead;
            }
            buffer[dwRead] = '\0';

            // 新浪API返回的是GBK编码，根据系统代码页进行转换
            if (g_data.m_system_code_page == 65001)
            {
                // UTF-8系统：需要将GBK数据转换为UTF-8
                int len = MultiByteToWideChar(936, 0, buffer.data(), -1, NULL, 0);
                if (len > 0)
                {
                    std::vector<wchar_t> wBuf(len);
                    MultiByteToWideChar(936, 0, buffer.data(), -1, wBuf.data(), len);
                    int len2 = WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, NULL, 0, NULL, NULL);
                    if (len2 > 0)
                    {
                        std::vector<char> finalBuf(len2);
                        WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, finalBuf.data(), len2, NULL, NULL);
                        result = finalBuf.data();
                    }
                }
            }
            else
            {
                result = buffer.data();
            }
            succeed = true;
        }
    }
    catch (CInternetException* e)
    {
        CCommon::WriteLog(L"request fail!", g_data.m_log_path.c_str());
        e->Delete();
        succeed = false;
    }
    return succeed;
}

std::wstring CCommon::URLEncode(const std::wstring& wstr)
{
    std::string str_utf8;
    std::wstring result{};
    wchar_t buff[4];
    str_utf8 = CCommon::UnicodeToStr(wstr.c_str(), true);
    for (const auto& ch : str_utf8)
    {
        if (ch == ' ')
            result.push_back(L'+');
        else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))
            result.push_back(static_cast<wchar_t>(ch));
        else if (ch == '-' || ch == '_' || ch == '.' || ch == '!' || ch == '~' || ch == '*'/* || ch == '\''*/ || ch == '(' || ch == ')')
            result.push_back(static_cast<wchar_t>(ch));
        else
        {
            swprintf_s(buff, L"%%%02X", static_cast<unsigned char>(ch));
            result += buff;
        }
    }
    return result;
}

void CCommon::WriteLog(const WORD w, LPCTSTR file_path)
{
    char buff[32];
    sprintf_s(buff, "%d", w);
    CCommon::WriteLog(buff, file_path);
}

void CCommon::WriteLog(const char* str_text, LPCTSTR file_path)
{
    // 使用 thread_local 替代 static，确保线程安全
    // 每个线程独立去重，避免竞态条件
    thread_local std::string last_text;

    // 过滤相同内容的日志
    if (last_text != str_text)
    {
        SYSTEMTIME cur_time;
        GetLocalTime(&cur_time);
        char buff[32];
        sprintf_s(buff, "%d/%.2d/%.2d %.2d:%.2d:%.2d.%.3d: ", cur_time.wYear, cur_time.wMonth, cur_time.wDay,
            cur_time.wHour, cur_time.wMinute, cur_time.wSecond, cur_time.wMilliseconds);

        // 使用静态互斥锁保护文件写入
        static std::mutex s_logMutex;
        std::lock_guard<std::mutex> lock(s_logMutex);

        std::ofstream file{ file_path, std::ios::app };
        file << buff;
        file << str_text << std::endl;

        last_text = str_text;
    }
}

void CCommon::WriteLog(const wchar_t* str_text, LPCTSTR file_path)
{
    WriteLog(UnicodeToStr(str_text, true).c_str(), file_path);
}

std::vector<std::string> CCommon::split(const std::string& str, const char pattern)
{
    std::vector<std::string> res;
    if (str.empty()) {
        return res;
    }
    if (str.find(pattern) == std::string::npos) {
        res.push_back(str);
        return res;
    }
    std::stringstream input(str);   //读取str到字符串流中
    std::string temp;
    //使用getline函数从字符串流中读取,遇到分隔符时停止,和从cin中读取类似
    //注意,getline默认是可以读取空格的
    int len = 0;
    while (getline(input, temp, pattern))
    {
        res.push_back(temp);
        len++;
    }
    res.resize(len);
    return res;
}

std::vector<std::string> CCommon::split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;

    if (delimiter.empty()) {
        tokens.push_back(str);
        return tokens;
    }

    size_t pos = 0;
    size_t prev = 0;

    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
        tokens.push_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.length();
    }

    // 添加最后一个片段
    tokens.push_back(str.substr(prev));

    return tokens;
}

std::wstring CCommon::vectorJoinString(const std::vector<std::wstring> data, const std::wstring& pattern)
{
    std::wstring str{};
    for (size_t index = 0; index < data.size(); index++)
    {
        if (index > 0)
            str.append(pattern);
        str.append(data[index]);
    }
    return str;
}

std::string CCommon::removeChar(const std::string &str, char ch)
{
    std::string result;
    for (char c : str)
    {
        if (c != ch)
        {
            result += c;
        }
    }
    return result;
}

std::string CCommon::removeStr(const std::string& str, const std::string& del)
{
    std::string result;

    if (del.empty()) {
        return str;
    }

    size_t pos = 0;
    size_t prev = 0;

    while ((pos = str.find(del, prev)) != std::string::npos) {
        result += str.substr(prev, pos - prev);
        prev = pos + del.length();
    }

    result += str.substr(prev);

    return result;
}

std::wstring CCommon::SmartShortName(const std::wstring& name)
{
    // 委托给 StockMappings 命名空间
    return StockMappings::SmartShortName(name);
}

std::wstring CCommon::SmartStockCode(const std::wstring& input)
{
    // 委托给 StockMappings 命名空间
    return StockMappings::SmartStockCode(input);
}

int CCommon::GetStockTypeIndex(const std::wstring& code)
{
    // 委托给 StockMappings 命名空间
    return StockMappings::GetStockTypeIndex(code);
}

int CCommon::CompareVersion(const std::wstring& v1, const std::wstring& v2)
{
    // 解析版本号，支持格式如 "1.14" 或 "1.14.0"
    std::vector<int> ver1, ver2;

    // 解析v1
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

    // 解析v2
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

bool CCommon::CheckForUpdate(const std::wstring& current_version, UpdateInfo& info)
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
        {
            return false;
        }

        // 读取响应
        std::string response;
        char buffer[4096];
        UINT nRead;
        while ((nRead = pFile->Read(buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[nRead] = '\0';
            response += buffer;
        }

        // 使用 yyjson 解析 GitHub releases JSON
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

            // 找到 Stock release
            latestStockTag = tagName;
            latestStockVersion = tagName + 7;  // 移除 "Stock_V" 前缀

            // 获取 html_url
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
                    const char* url = yyjson_get_str(urlVal);
                    if (!url) continue;

                    std::string assetUrl = url;
                    // 检查是否是 Stock 相关的 zip 文件
                    if (assetUrl.find("Stock") != std::string::npos &&
                        (assetUrl.find(".zip") != std::string::npos || assetUrl.find(".ZIP") != std::string::npos))
                    {
                        // 根据当前系统架构选择正确的版本
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

            break;  // 找到最新的 Stock release 就退出
        }

        if (latestStockVersion.empty())
            return false;

        info.version = StrToUnicode(latestStockVersion.c_str(), true);
        info.download_url = StrToUnicode(downloadUrl.c_str(), true);
        info.release_page_url = StrToUnicode(releasePageUrl.c_str(), true);

        // 如果没有找到 release page url，构建一个
        if (info.release_page_url.empty())
        {
            std::string defaultUrl = "https://github.com/Exlany/TrafficMonitorPlugins/releases/tag/" + latestStockTag;
            info.release_page_url = StrToUnicode(defaultUrl.c_str(), true);
        }

        // 比较版本号
        return CompareVersion(current_version, info.version) < 0;
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

std::wstring CCommon::GetPluginDirectory()
{
    // 获取当前 DLL 的路径
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

bool CCommon::DownloadFile(const std::wstring& url, const std::wstring& savePath)
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

bool CCommon::UnzipFile(const std::wstring& zipPath, const std::wstring& destDir)
{
    // 使用 Windows Shell API 解压 zip 文件
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

        // 选项: 4 = 不显示进度对话框, 16 = 对所有询问回答"是"
        vOptions.vt = VT_I4;
        vOptions.lVal = 4 | 16 | 1024;  // 1024 = 不显示错误UI

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

                    // 等待解压完成
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

bool CCommon::PerformUpdate(const std::wstring& zipUrl, const std::wstring& pluginDir)
{
    if (zipUrl.empty())
        return false;

    // 创建临时目录
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);

    std::wstring tempDir = tempPath;
    tempDir += L"StockPluginUpdate\\";

    // 创建临时目录
    CreateDirectoryW(tempDir.c_str(), NULL);

    // 下载 zip 文件
    std::wstring zipPath = tempDir + L"Stock_update.zip";
    if (!DownloadFile(zipUrl, zipPath))
    {
        WriteLog(L"Failed to download update file", g_data.m_log_path.c_str());
        return false;
    }

    // 解压到临时目录
    std::wstring extractDir = tempDir + L"extracted\\";
    CreateDirectoryW(extractDir.c_str(), NULL);

    if (!UnzipFile(zipPath, extractDir))
    {
        WriteLog(L"Failed to extract update file", g_data.m_log_path.c_str());
        DeleteFileW(zipPath.c_str());
        return false;
    }

    // 查找解压后的 Stock.dll
    std::wstring newDllPath;
    WIN32_FIND_DATAW findData;

    // 先在根目录查找
    std::wstring searchPath = extractDir + L"Stock.dll";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        newDllPath = extractDir + findData.cFileName;
        FindClose(hFind);
    }
    else
    {
        // 在子目录中查找
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
        WriteLog(L"Stock.dll not found in update package", g_data.m_log_path.c_str());
        DeleteFileW(zipPath.c_str());
        return false;
    }

    // 目标路径
    std::wstring targetDllPath = pluginDir + L"\\Stock.dll";
    std::wstring backupDllPath = pluginDir + L"\\Stock.dll.bak";

    // 删除旧的备份
    DeleteFileW(backupDllPath.c_str());

    // 将当前 dll 重命名为备份（dll 正在使用中，无法直接覆盖）
    // 使用 MoveFileEx 在重启后替换
    if (!MoveFileExW(targetDllPath.c_str(), backupDllPath.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        // 如果无法移动（文件被锁定），使用延迟替换
        MoveFileExW(targetDllPath.c_str(), backupDllPath.c_str(), MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
    }

    // 复制新 dll 到目标位置
    if (!CopyFileW(newDllPath.c_str(), targetDllPath.c_str(), FALSE))
    {
        // 如果无法复制（目标被锁定），使用延迟替换
        // 先复制到临时位置
        std::wstring pendingDllPath = pluginDir + L"\\Stock.dll.new";
        if (CopyFileW(newDllPath.c_str(), pendingDllPath.c_str(), FALSE))
        {
            // 设置重启后替换
            MoveFileExW(pendingDllPath.c_str(), targetDllPath.c_str(), MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
        }
        else
        {
            WriteLog(L"Failed to copy new dll", g_data.m_log_path.c_str());
            // 恢复备份
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

void CCommon::RestartTrafficMonitor()
{
    // 获取 TrafficMonitor.exe 的路径
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // 使用 ShellExecute 启动新实例
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    sei.lpFile = exePath;
    sei.nShow = SW_SHOW;

    if (ShellExecuteExW(&sei))
    {
        // 成功启动新实例后，退出当前进程
        // 延迟一小段时间确保新进程启动
        Sleep(500);
        ExitProcess(0);
    }
}

bool CCommon::IsNetworkAvailable()
{
    // 尝试连接到一个可靠的服务器来检测网络
    // 使用 GitHub 的 API 服务器
    try
    {
        auto sessionDeleter = [](CInternetSession* s) { if (s) { s->Close(); delete s; } };
        auto fileDeleter = [](CHttpFile* f) { if (f) { f->Close(); delete f; } };

        std::unique_ptr<CInternetSession, decltype(sessionDeleter)> session(
            new CInternetSession(L"Network Check", 1, INTERNET_OPEN_TYPE_PRECONFIG), sessionDeleter);
        session->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 3000);
        session->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 3000);

        std::unique_ptr<CHttpFile, decltype(fileDeleter)> pFile(
            (CHttpFile*)session->OpenURL(
                L"https://api.github.com",
                1,
                INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT,
                NULL, 0),
            fileDeleter);

        if (pFile)
        {
            DWORD dwStatusCode = 0;
            pFile->QueryInfoStatusCode(dwStatusCode);

            // 任何有效的 HTTP 响应都表示网络可用
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
