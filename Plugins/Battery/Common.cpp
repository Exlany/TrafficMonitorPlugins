#include "pch.h"
#include "Common.h"
#include <sstream>
#include <vector>
#include "DataManager.h"

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

std::wstring CCommon::TimeFormat(int seconds)
{
    int hour = seconds / 3600;
    int minute = seconds % 3600 / 60;
    //int second = seconds % 60;
    std::wstringstream wss;
    wss << hour << L' ' << g_data.StringRes(IDS_HOUR).GetString()
        << L' ' << minute << L' ' << g_data.StringRes(IDS_MINUTE).GetString()
        //<< L' ' << second << L' ' << g_data.StringRes(IDS_SECOND).GetString()
        ;
    return wss.str();
}
