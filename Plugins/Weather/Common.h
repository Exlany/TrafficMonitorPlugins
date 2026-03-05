#pragma once
#include <string>
#include <atomic>
class CCommon
{
public:
    //将const char*字符串转换成宽字符字符串
    static std::wstring StrToUnicode(const char* str, bool utf8 = false);

    static std::string UnicodeToStr(const wchar_t* wstr, bool utf8 = false);

    //获取URL的内容
    static bool GetURL(const std::wstring& url, std::string& result, bool utf8 = false, const std::wstring& user_agent = std::wstring());

    //将一个字符串转换成URL编码（以UTF8编码格式）
    static std::wstring URLEncode(const std::wstring& wstr);

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
