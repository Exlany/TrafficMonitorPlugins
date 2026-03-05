#pragma once

#include <string>
#include <vector>

namespace StockPlugin {
namespace Utils {

/// @brief 字符串工具类
/// @details 提供字符串编码转换、分割、URL编码等通用功能
class StringUtils
{
public:
    // 禁止实例化
    StringUtils() = delete;

    /// @brief 多字节字符串转 Unicode
    /// @param str 源字符串
    /// @param utf8 是否为 UTF-8 编码，false 则使用系统默认代码页
    /// @return Unicode 字符串
    static std::wstring ToUnicode(const char* str, bool utf8 = false);
    static std::wstring ToUnicode(const std::string& str, bool utf8 = false);

    /// @brief Unicode 转多字节字符串
    /// @param wstr 源 Unicode 字符串
    /// @param utf8 是否转为 UTF-8 编码，false 则使用系统默认代码页
    /// @return 多字节字符串
    static std::string FromUnicode(const wchar_t* wstr, bool utf8 = false);
    static std::string FromUnicode(const std::wstring& wstr, bool utf8 = false);

    /// @brief URL 编码
    /// @param wstr 要编码的字符串
    /// @return URL 编码后的字符串
    static std::wstring URLEncode(const std::wstring& wstr);

    /// @brief 按字符分割字符串
    /// @param str 源字符串
    /// @param delimiter 分隔符
    /// @return 分割后的字符串数组
    static std::vector<std::string> Split(const std::string& str, char delimiter);

    /// @brief 按字符串分割字符串
    /// @param str 源字符串
    /// @param delimiter 分隔符字符串
    /// @return 分割后的字符串数组
    static std::vector<std::string> Split(const std::string& str, const std::string& delimiter);

    /// @brief 连接字符串数组
    /// @param data 字符串数组
    /// @param separator 分隔符
    /// @return 连接后的字符串
    static std::wstring Join(const std::vector<std::wstring>& data, const std::wstring& separator);

    /// @brief 移除指定字符
    /// @param str 源字符串
    /// @param ch 要移除的字符
    /// @return 处理后的字符串
    static std::string RemoveChar(const std::string& str, char ch);

    /// @brief 移除指定子串
    /// @param str 源字符串
    /// @param substr 要移除的子串
    /// @return 处理后的字符串
    static std::string RemoveSubstr(const std::string& str, const std::string& substr);

    /// @brief 去除字符串首尾空白
    /// @param str 源字符串
    /// @return 处理后的字符串
    static std::string Trim(const std::string& str);
    static std::wstring Trim(const std::wstring& wstr);

    /// @brief 转换为小写
    static std::wstring ToLower(const std::wstring& wstr);

    /// @brief 转换为大写
    static std::wstring ToUpper(const std::wstring& wstr);
};

} // namespace Utils
} // namespace StockPlugin
