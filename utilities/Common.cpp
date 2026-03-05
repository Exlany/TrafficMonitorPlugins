#include "Common.h"
#include <windows.h>
#include <fstream>
#include <io.h>
#include <vector>

namespace utilities
{
    bool CCommon::GetFileContent(const wchar_t* file_path, std::string& contents_buff)
    {
        std::ifstream file{ file_path, std::ios::binary };
        if (file.fail())
            return false;
        file.seekg(0, file.end);
        size_t length = file.tellg();
        file.seekg(0, file.beg);

        std::vector<char> buff(length);
        file.read(buff.data(), length);
        file.close();

        contents_buff.assign(buff.data(), length);

        return true;
    }


    const char* CCommon::GetFileContent(const wchar_t* file_path, size_t& length)
    {
        std::ifstream file{ file_path, std::ios::binary };
        length = 0;
        if (file.fail())
            return nullptr;
        file.seekg(0, file.end);
        length = file.tellg();
        file.seekg(0, file.beg);

        char* buff = new (std::nothrow) char[length];
        if (buff == nullptr)
        {
            length = 0;
            return nullptr;
        }
        file.read(buff, length);
        file.close();

        return buff;
    }


    void CCommon::GetFiles(const wchar_t* path, std::vector<std::wstring>& files)
    {
        //�ļ����
        intptr_t hFile = 0;
        //�ļ���Ϣ
        _wfinddata_t fileinfo;
        if ((hFile = _wfindfirst(path, &fileinfo)) != -1)
        {
            do
            {
                std::wstring file_name(fileinfo.name);
                if (file_name != L"." && file_name != L"..")
                    files.push_back(file_name);  //���ļ�������(����"."��"..")
            } while (_wfindnext(hFile, &fileinfo) == 0);
        }
        _findclose(hFile);
    }


    /////////////////////////////////////////////////////////////////////////////////////////
    std::wstring StringHelper::StrToUnicode(const char* str, bool utf8 /*= false*/)
    {
        if (str == nullptr)
            return std::wstring();
        int size = MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, NULL, 0);
        if (size <= 0) return std::wstring();
        std::vector<wchar_t> buf(size + 1);
        MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, buf.data(), size);
        return std::wstring(buf.data());
    }

    std::string StringHelper::UnicodeToStr(const wchar_t* wstr, bool utf8 /*= false*/)
    {
        if (wstr == nullptr)
            return std::string();
        int size = WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, NULL, 0, NULL, NULL);
        if (size <= 0) return std::string();
        std::vector<char> buf(size + 1);
        WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, buf.data(), size, NULL, NULL);
        return std::string(buf.data());
    }

    bool StringHelper::StringReplace(std::wstring& str, const std::wstring& str_old, const std::wstring& str_new)
    {
        if (str.empty())
            return false;
        bool replaced{ false };
        size_t pos = 0;
        while ((pos = str.find(str_old, pos)) != std::wstring::npos)
        {
            str.replace(pos, str_old.length(), str_new);
            replaced = true;
            pos += str_new.length();    // ǰ�����滻����ַ���ĩβ
        }
        return replaced;
    }

    std::wstring StringHelper::StringFormat(const wchar_t* format_str, const std::initializer_list<CVariant>& paras)
    {
        std::wstring str = format_str;
        int index = 1;
        for (const auto& para : paras)
        {
            std::wstring format_str{ L"<%" + std::to_wstring(index) + L"%>" };
            StringHelper::StringReplace(str, format_str, para.ToString());
            index++;
        }
        return str;
    }

    template<class T>
    static void _StringNormalize(T& str)
    {
        if (str.empty()) return;

        int size = static_cast<int>(str.size());  //�ַ����ĳ���
        if (size < 0) return;
        int index1 = 0;     //�ַ����е�1�����ǿո������ַ���λ��
        int index2 = size - 1;  //�ַ��������һ�����ǿո������ַ���λ��
        while (index1 < size && str[index1] >= 0 && str[index1] <= 32)
            index1++;
        while (index2 >= 0 && str[index2] >= 0 && str[index2] <= 32)
            index2--;
        if (index1 > index2)    //���index1 > index2��˵���ַ���ȫ�ǿո������ַ�
            str.clear();
        else if (index1 == 0 && index2 == size - 1) //���index1��index2��ֵ�ֱ�Ϊ0��size - 1��˵���ַ���ǰ��û�пո������ַ���ֱ�ӷ���
            return;
        else
            str = str.substr(index1, index2 - index1 + 1);
    }


    void StringHelper::StringNormalize(std::wstring& str)
    {
        _StringNormalize(str);
    }

    void StringHelper::StringNormalize(std::string& str)
    {
        _StringNormalize(str);
    }

    //��һ���ַ����ָ�����ɸ��ַ���ģ������ֻ��Ϊstring��wstring��
    //str: ԭʼ�ַ���
    //div_ch: ���ڷָ���ַ�
    //result: ���շָ��Ľ��
    template<class T, class value_type>
    static void _StringSplit(const T& str, value_type div_ch, std::vector<T>& results, bool skip_empty = true, bool trim = true)
    {
        results.clear();
        size_t split_index = -1;
        size_t last_split_index = -1;
        while (true)
        {
            split_index = str.find(div_ch, split_index + 1);
            T split_str = str.substr(last_split_index + 1, split_index - last_split_index - 1);
            if (trim)
                _StringNormalize(split_str);
            if (!split_str.empty() || !skip_empty)
                results.push_back(split_str);
            if (split_index == std::wstring::npos)
                break;
            last_split_index = split_index;
        }
    }

    template<class T>
    static void _StringSplit(const T& str, const T& div_str, std::vector<T>& results, bool skip_empty = true, bool trim = true)
    {
        results.clear();
        size_t split_index = 0 - div_str.size();
        size_t last_split_index = 0 - div_str.size();
        while (true)
        {
            split_index = str.find(div_str, split_index + div_str.size());
            T split_str = str.substr(last_split_index + div_str.size(), split_index - last_split_index - div_str.size());
            if (trim)
                _StringNormalize(split_str);
            if (!split_str.empty() || !skip_empty)
                results.push_back(split_str);
            if (split_index == std::wstring::npos)
                break;
            last_split_index = split_index;
        }
    }

    void StringHelper::StringSplit(const std::wstring& str, wchar_t div_ch, std::vector<std::wstring>& results, bool skip_empty, bool trim)
    {
        _StringSplit(str, div_ch, results, skip_empty, trim);
    }

    void StringHelper::StringSplit(const std::string& str, char div_ch, std::vector<std::string>& results, bool skip_empty, bool trim)
    {
        _StringSplit(str, div_ch, results, skip_empty, trim);
    }

    void StringHelper::StringSplit(const std::wstring& str, const std::wstring& div_str, std::vector<std::wstring>& results, bool skip_empty, bool trim)
    {
        _StringSplit(str, div_str, results, skip_empty, trim);
    }

    void StringHelper::StringSplit(const std::string& str, const std::string& div_str, std::vector<std::string>& results, bool skip_empty, bool trim)
    {
        _StringSplit(str, div_str, results, skip_empty, trim);
    }

}