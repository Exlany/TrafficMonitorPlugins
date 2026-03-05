#pragma once
#include <string>
#include <iostream>
#include <cstring>
#include <string.h>
#include <atomic>

class CCommon
{
public:
    //将const char*字符串转换成宽字符字符串
    static std::wstring StrToUnicode(const char* str, bool utf8 = false);

    static std::string UnicodeToStr(const wchar_t* wstr, bool utf8 = false);

    //获取URL的内容
    static bool GetURL(const std::wstring& url, std::string& result, bool utf8 = false, LPCTSTR pstrAgent = NULL, const LPCTSTR headers = NULL, DWORD dwHeadersLength = 0);

    //将一个字符串转换成URL编码（以UTF8编码格式）
    static std::wstring URLEncode(const std::wstring& wstr);

    //将一个日志信息str_text写入到file_path文件中
    static void WriteLog(const WORD w, LPCTSTR file_path);
    static void WriteLog(const char* str_text, LPCTSTR file_path);
    static void WriteLog(const wchar_t* str_text, LPCTSTR file_path);
    // 字符串拆分
    static std::vector<std::string> split(const std::string& str, const char pattern);
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter);
    static std::wstring vectorJoinString(const std::vector<std::wstring> data, const std::wstring& pattern);
    static std::string removeChar(const std::string& str, char ch);
    static std::string removeStr(const std::string& str, const std::string& del);
    // 智能简称：自动缩短股票名称
    static std::wstring SmartShortName(const std::wstring& name);
    // 智能识别股票代码：自动纠错和补全前缀
    // 返回: 修正后的完整股票代码（带前缀）
    static std::wstring SmartStockCode(const std::wstring& input);
    // 获取股票代码的市场类型索引
    // 返回: 0=深证, 1=港股, 2=北交所, 3=上证, 4=美股个股, 5=美股指数, 6=OKX虚拟货币, 7=其他
    static int GetStockTypeIndex(const std::wstring& code);

    // 更新检查相关
    struct UpdateInfo {
        std::wstring version;
        std::wstring download_url;      // zip 下载地址
        std::wstring release_page_url;  // Release 页面地址
        std::wstring release_notes;
    };
    // 检查插件更新
    // current_version: 当前版本号
    // info: 输出参数，新版本信息
    // 返回: true表示有新版本
    static bool CheckForUpdate(const std::wstring& current_version, UpdateInfo& info);
    // 比较版本号
    // 返回: <0 表示v1<v2, 0表示相等, >0表示v1>v2
    static int CompareVersion(const std::wstring& v1, const std::wstring& v2);
    // 下载文件到指定路径
    static bool DownloadFile(const std::wstring& url, const std::wstring& savePath);
    // 解压 zip 文件到指定目录
    static bool UnzipFile(const std::wstring& zipPath, const std::wstring& destDir);
    // 执行自动更新
    // zipUrl: zip 下载地址
    // pluginDir: 插件目录
    // 返回: true表示更新成功（需要重启生效）
    static bool PerformUpdate(const std::wstring& zipUrl, const std::wstring& pluginDir);
    // 获取插件所在目录
    static std::wstring GetPluginDirectory();
    // 重启 TrafficMonitor
    static void RestartTrafficMonitor();
    // 检查网络是否可用
    static bool IsNetworkAvailable();
};


// RAII guard: sets an atomic<bool> flag to true on construction, false on destruction
class CFlagLocker
{
public:
    explicit CFlagLocker(std::atomic<bool>& flag)
        : m_flag(flag)
    {
        m_flag.store(true);
    }

    ~CFlagLocker()
    {
        m_flag.store(false);
    }

    CFlagLocker(const CFlagLocker&) = delete;
    CFlagLocker& operator=(const CFlagLocker&) = delete;

private:
    std::atomic<bool>& m_flag;
};
