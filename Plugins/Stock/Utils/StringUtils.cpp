#include "pch.h"
#include "StringUtils.h"
#include <Windows.h>
#include <sstream>
#include <algorithm>
#include <cwctype>

namespace StockPlugin {
namespace Utils {

std::wstring StringUtils::ToUnicode(const char* str, bool utf8)
{
    if (str == nullptr || str[0] == '\0')
        return std::wstring();

    UINT codePage = utf8 ? CP_UTF8 : CP_ACP;
    int size = MultiByteToWideChar(codePage, 0, str, -1, nullptr, 0);
    if (size <= 0)
        return std::wstring();

    std::vector<wchar_t> buf(static_cast<size_t>(size) + 1);
    MultiByteToWideChar(codePage, 0, str, -1, buf.data(), size);
    return std::wstring(buf.data());
}

std::wstring StringUtils::ToUnicode(const std::string& str, bool utf8)
{
    return ToUnicode(str.c_str(), utf8);
}

std::string StringUtils::FromUnicode(const wchar_t* wstr, bool utf8)
{
    if (wstr == nullptr || wstr[0] == L'\0')
        return std::string();

    UINT codePage = utf8 ? CP_UTF8 : CP_ACP;
    int size = WideCharToMultiByte(codePage, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return std::string();

    std::vector<char> buf(static_cast<size_t>(size) + 1);
    WideCharToMultiByte(codePage, 0, wstr, -1, buf.data(), size, nullptr, nullptr);
    return std::string(buf.data());
}

std::string StringUtils::FromUnicode(const std::wstring& wstr, bool utf8)
{
    return FromUnicode(wstr.c_str(), utf8);
}

std::wstring StringUtils::URLEncode(const std::wstring& wstr)
{
    if (wstr.empty())
        return wstr;

    std::string utf8Str = FromUnicode(wstr, true);
    std::wstring result;
    result.reserve(utf8Str.size() * 3);  // 预分配空间

    wchar_t hexBuf[4];
    for (unsigned char ch : utf8Str)
    {
        if (ch == ' ')
        {
            result.push_back(L'+');
        }
        else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))
        {
            result.push_back(static_cast<wchar_t>(ch));
        }
        else if (ch == '-' || ch == '_' || ch == '.' || ch == '!' || ch == '~' || ch == '*' || ch == '(' || ch == ')')
        {
            result.push_back(static_cast<wchar_t>(ch));
        }
        else
        {
            swprintf_s(hexBuf, L"%%%02X", ch);
            result += hexBuf;
        }
    }
    return result;
}

std::vector<std::string> StringUtils::Split(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    if (str.empty())
        return result;

    if (str.find(delimiter) == std::string::npos)
    {
        result.push_back(str);
        return result;
    }

    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter))
    {
        result.push_back(token);
    }
    return result;
}

std::vector<std::string> StringUtils::Split(const std::string& str, const std::string& delimiter)
{
    std::vector<std::string> result;
    if (str.empty())
        return result;

    if (delimiter.empty())
    {
        result.push_back(str);
        return result;
    }

    size_t pos = 0;
    size_t prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos)
    {
        result.push_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.length();
    }
    result.push_back(str.substr(prev));
    return result;
}

std::wstring StringUtils::Join(const std::vector<std::wstring>& data, const std::wstring& separator)
{
    if (data.empty())
        return std::wstring();

    std::wstring result;
    for (size_t i = 0; i < data.size(); ++i)
    {
        if (i > 0)
            result.append(separator);
        result.append(data[i]);
    }
    return result;
}

std::string StringUtils::RemoveChar(const std::string& str, char ch)
{
    std::string result;
    result.reserve(str.size());
    for (char c : str)
    {
        if (c != ch)
            result.push_back(c);
    }
    return result;
}

std::string StringUtils::RemoveSubstr(const std::string& str, const std::string& substr)
{
    if (str.empty() || substr.empty())
        return str;

    std::string result;
    result.reserve(str.size());

    size_t pos = 0;
    size_t prev = 0;
    while ((pos = str.find(substr, prev)) != std::string::npos)
    {
        result += str.substr(prev, pos - prev);
        prev = pos + substr.length();
    }
    result += str.substr(prev);
    return result;
}

std::string StringUtils::Trim(const std::string& str)
{
    if (str.empty())
        return str;

    size_t start = 0;
    size_t end = str.size();

    while (start < end && std::isspace(static_cast<unsigned char>(str[start])))
        ++start;

    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
        --end;

    return str.substr(start, end - start);
}

std::wstring StringUtils::Trim(const std::wstring& wstr)
{
    if (wstr.empty())
        return wstr;

    size_t start = 0;
    size_t end = wstr.size();

    while (start < end && iswspace(wstr[start]))
        ++start;

    while (end > start && iswspace(wstr[end - 1]))
        --end;

    return wstr.substr(start, end - start);
}

std::wstring StringUtils::ToLower(const std::wstring& wstr)
{
    std::wstring result = wstr;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

std::wstring StringUtils::ToUpper(const std::wstring& wstr)
{
    std::wstring result = wstr;
    std::transform(result.begin(), result.end(), result.begin(), ::towupper);
    return result;
}

} // namespace Utils
} // namespace StockPlugin
