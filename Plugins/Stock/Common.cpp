#include "pch.h"
#include "Common.h"
#include <afxinet.h>    //用于支持使用网络相关的类
#include <sstream>
#include "DataManager.h"
#include <shldisp.h>    // Shell API for unzip
#include <comdef.h>

std::wstring CCommon::StrToUnicode(const char* str, bool utf8)
{
    if (str == nullptr)
        return std::wstring();
    std::wstring result;
    int size;
    size = MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, NULL, 0);
    if (size <= 0) return std::wstring();
    wchar_t* str_unicode = new wchar_t[size + 1];
    MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, str_unicode, size);
    result.assign(str_unicode);
    delete[] str_unicode;
    return result;
}

std::string CCommon::UnicodeToStr(const wchar_t* wstr, bool utf8)
{
    if (wstr == nullptr)
        return std::string();
    std::string result;
    int size{ 0 };
    size = WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size <= 0) return std::string();
    char* str = new char[size + 1];
    WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, str, size, NULL, NULL);
    result.assign(str);
    delete[] str;
    return result;
}

bool CCommon::GetURL(const std::wstring& url, std::string& result, bool utf8, LPCTSTR user_agent, LPCTSTR headers, DWORD dwHeadersLength)
{
    bool succeed{ false };
    CInternetSession* pSession{};
    CHttpFile* pfile{};
    try
    {
        pSession = new CInternetSession(user_agent);
        //pfile = (CHttpFile*)pSession->OpenURL(url.c_str());
        //CCommon::WriteLog(L"request>>>>", g_data.m_log_path.c_str());
        pfile = (CHttpFile*)pSession->OpenURL(url.c_str(), 1, INTERNET_FLAG_TRANSFER_BINARY, headers, dwHeadersLength);  // 使用BINARY模式读取原始数据
        //CCommon::WriteLog(L"request<<<", g_data.m_log_path.c_str());
        DWORD dwStatusCode;
        pfile->QueryInfoStatusCode(dwStatusCode);
        if (dwStatusCode == HTTP_STATUS_OK)
        {
            DWORD dwTotal = pfile->GetLength();
            DWORD dwRead = 0;
            char* pBuffer = new char[dwTotal + 1];
            while (dwRead < dwTotal)
            {
                DWORD dwBytesRead = pfile->Read(pBuffer + dwRead, dwTotal - dwRead);
                if (dwBytesRead == 0)
                    break;
                dwRead += dwBytesRead;
            }
            pBuffer[dwRead] = '\0';

            // 新浪API返回的是GBK编码，根据系统代码页进行转换
            if (g_data.m_system_code_page == 65001)
            {
                // UTF-8系统：需要将GBK数据转换为UTF-8
                // 先从GBK转换为Unicode
                int len = MultiByteToWideChar(936, 0, pBuffer, -1, NULL, 0);  // 936 = GBK
                if (len > 0)
                {
                    wchar_t* pWBuffer = new wchar_t[len];
                    MultiByteToWideChar(936, 0, pBuffer, -1, pWBuffer, len);
                    // 再从Unicode转换为UTF-8
                    int len2 = WideCharToMultiByte(CP_UTF8, 0, pWBuffer, -1, NULL, 0, NULL, NULL);
                    if (len2 > 0)
                    {
                        char* pFinalBuffer = new char[len2];
                        WideCharToMultiByte(CP_UTF8, 0, pWBuffer, -1, pFinalBuffer, len2, NULL, NULL);
                        result = pFinalBuffer;
                        delete[] pFinalBuffer;
                    }
                    delete[] pWBuffer;
                }
            }
            else
            {
                // 非UTF-8系统（如GBK 936）：直接使用原始GBK数据
                result = pBuffer;
            }
            delete[] pBuffer;
            succeed = true;
        }
        pfile->Close();
        delete pfile;
        pSession->Close();
    }
    catch (CInternetException* e)
    {
        CCommon::WriteLog(L"request fail!", g_data.m_log_path.c_str());
        if (pfile != nullptr)
        {
            pfile->Close();
            delete pfile;
        }
        if (pSession != nullptr)
            pSession->Close();
        succeed = false;
        e->Delete();        //没有这句会造成内存泄露
        SAFE_DELETE(pSession);
    }
    SAFE_DELETE(pSession);
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
            swprintf_s(buff, L"%%%x", static_cast<unsigned char>(ch));
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
    static std::string last_text;
    //过滤相同内容的日志
    if (last_text != str_text)
    {
        SYSTEMTIME cur_time;
        GetLocalTime(&cur_time);
        char buff[32];
        sprintf_s(buff, "%d/%.2d/%.2d %.2d:%.2d:%.2d.%.3d: ", cur_time.wYear, cur_time.wMonth, cur_time.wDay,
            cur_time.wHour, cur_time.wMinute, cur_time.wSecond, cur_time.wMilliseconds);
        std::ofstream file{ file_path, std::ios::app };  //以追加的方式打开日志文件
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
    if (str.size() <= 0) {
        return res;
    }
    if (str.find(pattern) == -1) {
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

std::string CCommon::removeStr(const std::string str, const std::string del)
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
    if (name.empty())
        return name;

    // 1. 常见股票名称直接映射（优先级最高）
    static const std::map<std::wstring, std::wstring> directMap = {
        // ========== A股大盘指数 ==========
        {L"上证指数", L"上证"}, {L"深证成指", L"深成"}, {L"创业板指", L"创业"},
        {L"沪深300", L"沪深"}, {L"中证500", L"中证"}, {L"科创50", L"科创"},
        {L"上证50", L"上证50"}, {L"中证1000", L"中证千"},

        // ========== A股银行 ==========
        {L"招商银行", L"招行"}, {L"工商银行", L"工行"}, {L"建设银行", L"建行"},
        {L"农业银行", L"农行"}, {L"中国银行", L"中行"}, {L"交通银行", L"交行"},
        {L"兴业银行", L"兴业"}, {L"浦发银行", L"浦发"}, {L"民生银行", L"民生"},
        {L"平安银行", L"平银"}, {L"光大银行", L"光大"}, {L"华夏银行", L"华夏"},
        {L"中信银行", L"中信"}, {L"北京银行", L"北银"}, {L"宁波银行", L"宁波"},
        {L"南京银行", L"南银"}, {L"杭州银行", L"杭银"}, {L"成都银行", L"成银"},

        // ========== A股保险 ==========
        {L"中国平安", L"平安"}, {L"中国人寿", L"人寿"}, {L"中国太保", L"太保"},
        {L"新华保险", L"新华"}, {L"中国人保", L"人保"},

        // ========== A股证券 ==========
        {L"中信证券", L"中信"}, {L"华泰证券", L"华泰"}, {L"国泰君安", L"君安"},
        {L"海通证券", L"海通"}, {L"广发证券", L"广发"}, {L"招商证券", L"招证"},
        {L"东方财富", L"东财"}, {L"中金公司", L"中金"},

        // ========== A股白酒 ==========
        {L"贵州茅台", L"茅台"}, {L"五粮液", L"五粮液"}, {L"泸州老窖", L"泸州"},
        {L"山西汾酒", L"汾酒"}, {L"洋河股份", L"洋河"}, {L"古井贡酒", L"古井"},
        {L"今世缘", L"今世缘"}, {L"舍得酒业", L"舍得"},

        // ========== A股科技 ==========
        {L"比亚迪", L"比亚迪"}, {L"宁德时代", L"宁德"}, {L"立讯精密", L"立讯"},
        {L"海康威视", L"海康"}, {L"中芯国际", L"中芯"}, {L"韦尔股份", L"韦尔"},
        {L"北方华创", L"北华创"}, {L"中微公司", L"中微"}, {L"澜起科技", L"澜起"},

        // ========== A股其他知名 ==========
        {L"中国石油", L"中油"}, {L"中国石化", L"中化"}, {L"中国神华", L"神华"},
        {L"中国移动", L"移动"}, {L"中国电信", L"电信"}, {L"中国联通", L"联通"},
        {L"格力电器", L"格力"}, {L"美的集团", L"美的"}, {L"海尔智家", L"海尔"},
        {L"万科A", L"万科"}, {L"保利发展", L"保利"}, {L"恒瑞医药", L"恒瑞"},
        {L"药明康德", L"药明"}, {L"迈瑞医疗", L"迈瑞"}, {L"隆基绿能", L"隆基"},
        {L"长江电力", L"长电"}, {L"紫金矿业", L"紫金"}, {L"中国中免", L"中免"},
        {L"海天味业", L"海天"}, {L"伊利股份", L"伊利"}, {L"中国建筑", L"中建"},
        {L"中国铁建", L"中铁建"}, {L"中国中车", L"中车"}, {L"中国交建", L"中交"},

        // ========== 港股科技 ==========
        {L"腾讯控股", L"腾讯"}, {L"阿里巴巴", L"阿里"}, {L"美团-W", L"美团"},
        {L"京东集团", L"京东"}, {L"百度集团", L"百度"}, {L"小米集团", L"小米"},
        {L"网易-S", L"网易"}, {L"快手-W", L"快手"}, {L"哔哩哔哩", L"B站"},
        {L"携程集团", L"携程"}, {L"商汤-W", L"商汤"}, {L"金蝶国际", L"金蝶"},
        {L"联想集团", L"联想"}, {L"中兴通讯", L"中兴"},

        // ========== 港股金融 ==========
        {L"汇丰控股", L"汇丰"}, {L"友邦保险", L"友邦"}, {L"香港交易所", L"港交所"},
        {L"中国财险", L"财险"}, {L"众安在线", L"众安"},

        // ========== 港股消费 ==========
        {L"蒙牛乳业", L"蒙牛"}, {L"农夫山泉", L"农夫"}, {L"海底捞", L"海底捞"},
        {L"安踏体育", L"安踏"}, {L"李宁", L"李宁"}, {L"华润啤酒", L"华润"},
        {L"青岛啤酒", L"青啤"}, {L"百威亚太", L"百威"},

        // ========== 港股地产 ==========
        {L"华润置地", L"华润地"}, {L"龙湖集团", L"龙湖"}, {L"碧桂园", L"碧桂园"},
        {L"中国海外发展", L"中海"}, {L"新城发展", L"新城"},

        // ========== 美股科技巨头 ==========
        {L"苹果", L"苹果"}, {L"微软", L"微软"}, {L"谷歌", L"谷歌"},
        {L"亚马逊", L"亚马逊"}, {L"Meta", L"Meta"}, {L"英伟达", L"英伟达"},
        {L"特斯拉", L"特斯拉"}, {L"奈飞", L"奈飞"}, {L"英特尔", L"英特尔"},
        {L"AMD", L"AMD"}, {L"高通", L"高通"}, {L"博通", L"博通"},
        {L"甲骨文", L"甲骨文"}, {L"思科", L"思科"}, {L"IBM", L"IBM"},
        {L"Adobe", L"Adobe"}, {L"Salesforce", L"SF"},

        // ========== 美股中概股 ==========
        {L"阿里巴巴集团", L"阿里"}, {L"拼多多", L"拼多多"}, {L"网易公司", L"网易"},
        {L"京东", L"京东"}, {L"百度", L"百度"}, {L"蔚来", L"蔚来"},
        {L"小鹏汽车", L"小鹏"}, {L"理想汽车", L"理想"}, {L"哔哩哔哩", L"B站"},
        {L"爱奇艺", L"爱奇艺"}, {L"唯品会", L"唯品会"}, {L"新东方", L"新东方"},
        {L"好未来", L"好未来"}, {L"贝壳", L"贝壳"}, {L"满帮", L"满帮"},

        // ========== 美股其他知名 ==========
        {L"伯克希尔", L"伯克希尔"}, {L"摩根大通", L"摩根"}, {L"美国银行", L"美银"},
        {L"富国银行", L"富国"}, {L"高盛", L"高盛"}, {L"摩根士丹利", L"大摩"},
        {L"可口可乐", L"可口"}, {L"百事可乐", L"百事"}, {L"麦当劳", L"麦当劳"},
        {L"星巴克", L"星巴克"}, {L"耐克", L"耐克"}, {L"迪士尼", L"迪士尼"},
        {L"强生", L"强生"}, {L"辉瑞", L"辉瑞"}, {L"默沙东", L"默沙东"},
        {L"埃克森美孚", L"埃克森"}, {L"雪佛龙", L"雪佛龙"},

        // ========== 热门ETF/指数基金 ==========
        // 宽基ETF
        {L"沪深300ETF", L"300ETF"}, {L"中证500ETF", L"500ETF"},
        {L"创业板ETF", L"创业ETF"}, {L"科创50ETF", L"科创ETF"},
        {L"上证50ETF", L"50ETF"}, {L"中证1000ETF", L"1000ETF"},
        {L"上证180ETF", L"180ETF"}, {L"深证100ETF", L"深100ETF"},
        {L"中证100ETF", L"100ETF"}, {L"MSCI中国A50互联互通ETF", L"A50ETF"},

        // 跨境ETF
        {L"恒生ETF", L"恒生ETF"}, {L"H股ETF", L"H股ETF"},
        {L"恒生科技ETF", L"恒科ETF"}, {L"恒生互联网ETF", L"恒网ETF"},
        {L"纳斯达克ETF", L"纳指ETF"}, {L"纳指ETF", L"纳指ETF"},
        {L"标普500ETF", L"标普ETF"}, {L"日经ETF", L"日经ETF"},
        {L"德国ETF", L"德国ETF"}, {L"法国ETF", L"法国ETF"},

        // 行业ETF
        {L"证券ETF", L"券商ETF"}, {L"券商ETF", L"券商ETF"},
        {L"银行ETF", L"银行ETF"}, {L"保险ETF", L"保险ETF"},
        {L"医药ETF", L"医药ETF"}, {L"医疗ETF", L"医疗ETF"},
        {L"消费ETF", L"消费ETF"}, {L"食品饮料ETF", L"食品ETF"},
        {L"白酒ETF", L"白酒ETF"}, {L"家电ETF", L"家电ETF"},
        {L"新能源ETF", L"新能ETF"}, {L"光伏ETF", L"光伏ETF"},
        {L"新能源车ETF", L"新车ETF"}, {L"电池ETF", L"电池ETF"},
        {L"半导体ETF", L"芯片ETF"}, {L"芯片ETF", L"芯片ETF"},
        {L"科技ETF", L"科技ETF"}, {L"通信ETF", L"通信ETF"},
        {L"计算机ETF", L"计算ETF"}, {L"软件ETF", L"软件ETF"},
        {L"人工智能ETF", L"AI ETF"}, {L"云计算ETF", L"云计ETF"},
        {L"军工ETF", L"军工ETF"}, {L"国防ETF", L"国防ETF"},
        {L"房地产ETF", L"地产ETF"}, {L"基建ETF", L"基建ETF"},
        {L"钢铁ETF", L"钢铁ETF"}, {L"煤炭ETF", L"煤炭ETF"},
        {L"有色ETF", L"有色ETF"}, {L"化工ETF", L"化工ETF"},
        {L"农业ETF", L"农业ETF"}, {L"畜牧ETF", L"畜牧ETF"},
        {L"旅游ETF", L"旅游ETF"}, {L"传媒ETF", L"传媒ETF"},
        {L"游戏ETF", L"游戏ETF"}, {L"教育ETF", L"教育ETF"},
        {L"环保ETF", L"环保ETF"}, {L"水利ETF", L"水利ETF"},

        // 商品ETF
        {L"黄金ETF", L"黄金ETF"}, {L"白银ETF", L"白银ETF"},
        {L"原油ETF", L"原油ETF"}, {L"豆粕ETF", L"豆粕ETF"},
        {L"有色金属ETF", L"有色ETF"}, {L"能源化工ETF", L"能化ETF"},

        // 策略ETF
        {L"红利ETF", L"红利ETF"}, {L"价值ETF", L"价值ETF"},
        {L"成长ETF", L"成长ETF"}, {L"低波ETF", L"低波ETF"},
        {L"质量ETF", L"质量ETF"}, {L"动量ETF", L"动量ETF"},

        // 债券ETF
        {L"国债ETF", L"国债ETF"}, {L"企债ETF", L"企债ETF"},
        {L"可转债ETF", L"转债ETF"}, {L"信用债ETF", L"信用ETF"},

        // 华夏系列ETF（常见）
        {L"华夏上证50ETF", L"50ETF"}, {L"华夏沪深300ETF", L"300ETF"},
        {L"华夏恒生ETF", L"恒生ETF"}, {L"华夏恒生科技ETF", L"恒科ETF"},

        // 易方达系列ETF
        {L"易方达沪深300ETF", L"300ETF"}, {L"易方达创业板ETF", L"创业ETF"},
        {L"易方达中证500ETF", L"500ETF"}, {L"易方达科创50ETF", L"科创ETF"},

        // 南方系列ETF
        {L"南方中证500ETF", L"500ETF"}, {L"南方中证1000ETF", L"1000ETF"},

        // ========== 热门基金 ==========
        {L"易方达蓝筹", L"易蓝筹"}, {L"易方达中小盘", L"易中小"},
        {L"招商中证白酒", L"招白酒"}, {L"天弘中证银行", L"天银行"},
        {L"华夏上证50", L"华50"}, {L"南方中证500", L"南500"},
        {L"广发纳斯达克", L"广纳指"}, {L"博时黄金", L"博黄金"},
        {L"华安黄金易", L"华黄金"}, {L"诺安成长", L"诺成长"},
        {L"中欧医疗健康", L"中欧医"}, {L"工银瑞信前沿医疗", L"工银医"},
        {L"景顺长城新兴成长", L"景顺成长"}, {L"兴全合润", L"兴合润"}
    };

    auto it = directMap.find(name);
    if (it != directMap.end())
        return it->second;

    std::wstring result = name;

    // 2. 移除常见前缀（地名）
    static const std::vector<std::wstring> prefixes = {
        L"中国", L"中华", L"全国", L"国家",
        L"上海", L"深圳", L"北京", L"广州", L"天津", L"重庆",
        L"江苏", L"浙江", L"山东", L"四川", L"湖北", L"湖南",
        L"河南", L"河北", L"福建", L"安徽", L"陕西", L"辽宁",
        L"广东", L"云南", L"贵州", L"山西", L"吉林", L"黑龙江",
        L"江西", L"海南", L"甘肃", L"青海", L"宁夏", L"新疆",
        L"西藏", L"内蒙古", L"广西"
    };

    for (const auto& prefix : prefixes)
    {
        if (result.length() > prefix.length() &&
            result.substr(0, prefix.length()) == prefix)
        {
            result = result.substr(prefix.length());
            break;
        }
    }

    // 3. 移除常见后缀
    static const std::vector<std::wstring> suffixes_to_remove = {
        L"股份有限公司", L"有限责任公司", L"有限公司",
        L"集团股份", L"集团公司", L"集团",
        L"控股股份", L"控股公司", L"控股",
        L"实业", L"企业"
    };

    for (const auto& suffix : suffixes_to_remove)
    {
        if (result.length() > suffix.length())
        {
            size_t pos = result.rfind(suffix);
            if (pos != std::wstring::npos && pos + suffix.length() == result.length())
            {
                result = result.substr(0, pos);
                break;
            }
        }
    }

    // 4. 移除A/B/H股标识
    if (result.length() > 1)
    {
        wchar_t last = result.back();
        if (last == L'A' || last == L'B' || last == L'H' ||
            last == L'Ａ' || last == L'Ｂ' || last == L'Ｈ')
        {
            result = result.substr(0, result.length() - 1);
        }
    }

    // 5. ETF/LOF/基金特殊处理
    bool isETF = (name.find(L"ETF") != std::wstring::npos || name.find(L"etf") != std::wstring::npos);
    bool isLOF = (name.find(L"LOF") != std::wstring::npos || name.find(L"lof") != std::wstring::npos);
    bool isFund = isETF || isLOF ||
                  name.find(L"基金") != std::wstring::npos ||
                  name.find(L"联接") != std::wstring::npos;

    if (isFund)
    {
        // ETF/基金名称简化规则
        std::wstring fundResult = result;

        // 移除基金公司名称前缀
        static const std::vector<std::wstring> fundCompanies = {
            L"华夏", L"易方达", L"南方", L"广发", L"博时", L"嘉实", L"富国", L"汇添富",
            L"招商", L"工银", L"华安", L"天弘", L"中欧", L"景顺", L"兴全", L"交银",
            L"鹏华", L"国泰", L"华宝", L"银华", L"诺安", L"大成", L"建信", L"中银",
            L"平安", L"万家", L"长城", L"国投", L"华泰", L"申万", L"海富通", L"长盛"
        };

        for (const auto& company : fundCompanies)
        {
            if (fundResult.length() > company.length() &&
                fundResult.substr(0, company.length()) == company)
            {
                fundResult = fundResult.substr(company.length());
                break;
            }
        }

        // 移除后缀
        static const std::vector<std::wstring> fundSuffixes = {
            L"联接A", L"联接C", L"联接", L"ETF联接", L"LOF",
            L"(QDII)", L"QDII", L"增强", L"指数"
        };

        for (const auto& suffix : fundSuffixes)
        {
            size_t pos = fundResult.rfind(suffix);
            if (pos != std::wstring::npos && pos + suffix.length() == fundResult.length())
            {
                fundResult = fundResult.substr(0, pos);
            }
        }

        // 移除末尾的 A/C 份额标识，但要确保前面不是英文字母（避免把 AI 变成 I）
        if (fundResult.length() > 1)
        {
            wchar_t last = fundResult.back();
            wchar_t prev = fundResult[fundResult.length() - 2];
            if ((last == L'A' || last == L'C') && !iswalpha(prev))
            {
                fundResult = fundResult.substr(0, fundResult.length() - 1);
            }
        }

        // 保留 ETF 标识
        if (isETF)
        {
            // 移除名称中的 ETF 字样，稍后统一添加
            size_t etfPos = fundResult.find(L"ETF");
            if (etfPos != std::wstring::npos)
                fundResult = fundResult.substr(0, etfPos) + fundResult.substr(etfPos + 3);
            etfPos = fundResult.find(L"etf");
            if (etfPos != std::wstring::npos)
                fundResult = fundResult.substr(0, etfPos) + fundResult.substr(etfPos + 3);

            // 商品ETF特殊处理：提取核心商品名称
            static const std::vector<std::pair<std::wstring, std::wstring>> commodityKeywords = {
                {L"黄金", L"黄金"}, {L"白银", L"白银"}, {L"原油", L"原油"},
                {L"豆粕", L"豆粕"}, {L"有色", L"有色"}, {L"能源", L"能源"},
                {L"铜", L"铜"}, {L"铝", L"铝"}, {L"锌", L"锌"}
            };
            for (const auto& kw : commodityKeywords)
            {
                if (fundResult.find(kw.first) != std::wstring::npos)
                {
                    fundResult = kw.second;
                    break;
                }
            }

            // 限制核心名称长度
            if (fundResult.length() > 4)
                fundResult = fundResult.substr(0, 4);

            // 添加 ETF 后缀
            if (!fundResult.empty())
                fundResult += L"ETF";
        }
        else if (isLOF)
        {
            if (fundResult.length() > 4)
                fundResult = fundResult.substr(0, 4);
            fundResult += L"LOF";
        }
        else
        {
            // 普通基金
            if (fundResult.length() > 5)
                fundResult = fundResult.substr(0, 5);
        }

        if (!fundResult.empty())
            return fundResult;
    }

    // 6. 限制最大长度（4个汉字）
    if (result.length() > 4)
        result = result.substr(0, 4);

    // 如果处理后为空，返回原名称的前4个字符
    if (result.empty())
        return name.length() > 4 ? name.substr(0, 4) : name;

    return result;
}

// 常见美股指数映射表（用户输入 -> API代码）
static const std::map<std::wstring, std::wstring> usIndexMap = {
    // 纳斯达克
    {L"nasdaq", L"int_nasdaq"}, {L"NASDAQ", L"int_nasdaq"},
    {L"纳斯达克", L"int_nasdaq"}, {L"纳指", L"int_nasdaq"},
    {L"ixic", L"int_nasdaq"}, {L"IXIC", L"int_nasdaq"},
    // 道琼斯
    {L"dji", L"int_dji"}, {L"DJI", L"int_dji"},
    {L"道琼斯", L"int_dji"}, {L"道指", L"int_dji"},
    {L"djia", L"int_dji"}, {L"DJIA", L"int_dji"},
    // 标普500
    {L"sp500", L"int_sp500"}, {L"SP500", L"int_sp500"},
    {L"标普", L"int_sp500"}, {L"标普500", L"int_sp500"},
    {L"spx", L"int_sp500"}, {L"SPX", L"int_sp500"},
    // 美元指数
    {L"dxy", L"int_dxy"}, {L"DXY", L"int_dxy"},
    {L"美元指数", L"int_dxy"}, {L"美指", L"int_dxy"},
    {L"usdx", L"int_dxy"}, {L"USDX", L"int_dxy"},
    // 恐慌指数
    {L"vix", L"int_vix"}, {L"VIX", L"int_vix"},
    {L"恐慌指数", L"int_vix"},
    // 罗素2000
    {L"rut", L"int_rut"}, {L"RUT", L"int_rut"},
    {L"罗素", L"int_rut"}, {L"罗素2000", L"int_rut"},
    // 费城半导体
    {L"sox", L"int_sox"}, {L"SOX", L"int_sox"},
    {L"费半", L"int_sox"}, {L"半导体指数", L"int_sox"},
    // 英国富时100
    {L"ftse", L"int_ftse"}, {L"FTSE", L"int_ftse"},
    {L"富时", L"int_ftse"}, {L"富时100", L"int_ftse"},
    // 德国DAX
    {L"dax", L"int_dax"}, {L"DAX", L"int_dax"},
    {L"德指", L"int_dax"},
    // 法国CAC40
    {L"cac", L"int_cac"}, {L"CAC", L"int_cac"},
    {L"cac40", L"int_cac"}, {L"CAC40", L"int_cac"},
    // 日经225
    {L"n225", L"int_n225"}, {L"N225", L"int_n225"},
    {L"日经", L"int_n225"}, {L"日经225", L"int_n225"},
    {L"nikkei", L"int_n225"}, {L"NIKKEI", L"int_n225"},
    // 韩国KOSPI
    {L"kospi", L"int_kospi"}, {L"KOSPI", L"int_kospi"},
    {L"韩国综合", L"int_kospi"},
    // 台湾加权
    {L"twii", L"int_twii"}, {L"TWII", L"int_twii"},
    {L"台湾加权", L"int_twii"}, {L"台指", L"int_twii"},
    // 澳洲ASX200
    {L"asx", L"int_asx200"}, {L"ASX", L"int_asx200"},
    {L"asx200", L"int_asx200"}, {L"ASX200", L"int_asx200"},
    // 印度SENSEX
    {L"sensex", L"int_sensex"}, {L"SENSEX", L"int_sensex"},
    {L"印度", L"int_sensex"},
    // 黄金
    {L"gold", L"hf_GC"}, {L"GOLD", L"hf_GC"},
    {L"黄金", L"hf_GC"}, {L"伦敦金", L"hf_GC"},
    // 白银
    {L"silver", L"hf_SI"}, {L"SILVER", L"hf_SI"},
    {L"白银", L"hf_SI"},
    // 原油
    {L"oil", L"hf_CL"}, {L"OIL", L"hf_CL"},
    {L"原油", L"hf_CL"}, {L"wti", L"hf_CL"}, {L"WTI", L"hf_CL"},
    // 布伦特原油
    {L"brent", L"hf_OIL"}, {L"BRENT", L"hf_OIL"},
    {L"布伦特", L"hf_OIL"},
};

// 常见美股代码映射（简写 -> 完整代码）
static const std::map<std::wstring, std::wstring> usStockMap = {
    {L"aapl", L"gb_aapl"}, {L"AAPL", L"gb_aapl"}, {L"苹果", L"gb_aapl"},
    {L"tsla", L"gb_tsla"}, {L"TSLA", L"gb_tsla"}, {L"特斯拉", L"gb_tsla"},
    {L"nvda", L"gb_nvda"}, {L"NVDA", L"gb_nvda"}, {L"英伟达", L"gb_nvda"},
    {L"msft", L"gb_msft"}, {L"MSFT", L"gb_msft"}, {L"微软", L"gb_msft"},
    {L"goog", L"gb_goog"}, {L"GOOG", L"gb_goog"}, {L"谷歌", L"gb_goog"},
    {L"googl", L"gb_googl"}, {L"GOOGL", L"gb_googl"},
    {L"amzn", L"gb_amzn"}, {L"AMZN", L"gb_amzn"}, {L"亚马逊", L"gb_amzn"},
    {L"meta", L"gb_meta"}, {L"META", L"gb_meta"},
    {L"baba", L"gb_baba"}, {L"BABA", L"gb_baba"}, {L"阿里", L"gb_baba"},
    {L"pdd", L"gb_pdd"}, {L"PDD", L"gb_pdd"}, {L"拼多多", L"gb_pdd"},
    {L"jd", L"gb_jd"}, {L"JD", L"gb_jd"}, {L"京东", L"gb_jd"},
    {L"ntes", L"gb_ntes"}, {L"NTES", L"gb_ntes"}, {L"网易", L"gb_ntes"},
    {L"bidu", L"gb_bidu"}, {L"BIDU", L"gb_bidu"}, {L"百度", L"gb_bidu"},
    {L"nio", L"gb_nio"}, {L"NIO", L"gb_nio"}, {L"蔚来", L"gb_nio"},
    {L"xpev", L"gb_xpev"}, {L"XPEV", L"gb_xpev"}, {L"小鹏", L"gb_xpev"},
    {L"li", L"gb_li"}, {L"LI", L"gb_li"}, {L"理想", L"gb_li"},
    {L"amd", L"gb_amd"}, {L"AMD", L"gb_amd"},
    {L"intc", L"gb_intc"}, {L"INTC", L"gb_intc"}, {L"英特尔", L"gb_intc"},
    {L"nflx", L"gb_nflx"}, {L"NFLX", L"gb_nflx"}, {L"奈飞", L"gb_nflx"},
    {L"coin", L"gb_coin"}, {L"COIN", L"gb_coin"},
};

std::wstring CCommon::SmartStockCode(const std::wstring& input)
{
    if (input.empty())
        return input;

    std::wstring code = input;

    // 1. 检查是否已有前缀
    if (code.find(L"sz") == 0 || code.find(L"sh") == 0 || code.find(L"bj") == 0 ||
        code.find(L"rt_hk") == 0 || code.find(L"gb_") == 0 || code.find(L"int_") == 0 ||
        code.find(L"okx_") == 0 || code.find(L"hf_") == 0)
    {
        return code;
    }

    // 2. 检查是否是已知的指数或期货
    auto itIndex = usIndexMap.find(code);
    if (itIndex != usIndexMap.end())
        return itIndex->second;

    // 3. 检查是否是已知的美股代码
    auto itStock = usStockMap.find(code);
    if (itStock != usStockMap.end())
        return itStock->second;

    // 4. 检查是否是OKX虚拟货币格式
    if (code.find(L'-') != std::wstring::npos)
    {
        std::wstring upper = code;
        for (auto& c : upper) c = towupper(c);
        if (upper.find(L"USDT") != std::wstring::npos || upper.find(L"USD") != std::wstring::npos ||
            upper.find(L"BTC") != std::wstring::npos || upper.find(L"ETH") != std::wstring::npos)
        {
            return L"okx_" + code;
        }
    }

    // 5. 检查是否全是数字
    bool allDigits = true;
    for (const auto& c : code)
    {
        if (!iswdigit(c))
        {
            allDigits = false;
            break;
        }
    }

    if (allDigits)
    {
        size_t len = code.length();
        if (len == 6)
        {
            wchar_t first = code[0];
            // 上证: 6开头(A股主板)、5开头(ETF/LOF基金)、9开头(B股)
            if (first == L'6' || first == L'5' || first == L'9')
                return L"sh" + code;
            // 深证: 0开头(主板/中小板)、2开头(B股)、3开头(创业板)、1开头(基金/债券)
            if (first == L'0' || first == L'2' || first == L'3' || first == L'1')
                return L"sz" + code;
            // 北交所: 8开头、4开头
            if (first == L'8' || first == L'4')
                return L"bj" + code;
        }
        else if (len == 5)
        {
            // 港股
            return L"rt_hk" + code;
        }
    }
    else
    {
        // 包含字母，默认为美股个股
        std::wstring lower = code;
        for (auto& c : lower) c = towlower(c);
        return L"gb_" + lower;
    }

    return code;
}

int CCommon::GetStockTypeIndex(const std::wstring& code)
{
    if (code.find(L"sz") == 0)
        return 0; // 深证
    if (code.find(L"rt_hk") == 0)
        return 1; // 港股
    if (code.find(L"bj") == 0)
        return 2; // 北交所
    if (code.find(L"sh") == 0)
        return 3; // 上证
    if (code.find(L"gb_") == 0)
        return 4; // 美股个股
    if (code.find(L"int_") == 0 || code.find(L"hf_") == 0)
        return 5; // 美股指数/期货
    if (code.find(L"okx_") == 0)
        return 6; // OKX虚拟货币
    return 7; // 其他
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

        CInternetSession session(L"Stock Plugin Update Checker");
        session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 10000);
        session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 10000);

        CHttpFile* pFile = (CHttpFile*)session.OpenURL(url.c_str(), 1,
            INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE,
            L"Accept: application/vnd.github.v3+json", 0);

        if (!pFile)
            return false;

        DWORD dwStatusCode;
        pFile->QueryInfoStatusCode(dwStatusCode);

        if (dwStatusCode != HTTP_STATUS_OK)
        {
            pFile->Close();
            delete pFile;
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

        pFile->Close();
        delete pFile;
        session.Close();

        // 查找最新的 Stock_V* release
        // 遍历所有 release，找到 tag_name 以 Stock_V 开头的
        std::string latestStockTag;
        std::string latestStockVersion;
        std::string downloadUrl;
        std::string releasePageUrl;
        size_t searchPos = 0;

        while (true)
        {
            // 查找 "tag_name"
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

            // 检查是否是 Stock 标签
            if (tagName.find("Stock_V") == 0)
            {
                // 提取版本号 (Stock_V1.18 -> 1.18)
                std::string version = tagName.substr(7);  // 移除 "Stock_V" 前缀

                latestStockTag = tagName;
                latestStockVersion = version;

                // 查找这个 release 的 html_url
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
                // 格式: "browser_download_url":"https://...Stock.zip"
                size_t assetsPos = response.find("\"assets\"", searchPos);
                if (assetsPos != std::string::npos)
                {
                    // 在 assets 数组中查找 Stock 相关的 zip 文件
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
                            // 检查是否是 Stock 相关的 zip 文件
                            if (assetUrl.find("Stock") != std::string::npos &&
                                (assetUrl.find(".zip") != std::string::npos || assetUrl.find(".ZIP") != std::string::npos))
                            {
                                // 根据当前系统架构选择正确的版本
                                #if defined(_M_X64) || defined(__x86_64__)
                                    // 64位系统优先选择 x64 版本
                                    if (assetUrl.find("x64") != std::string::npos)
                                    {
                                        downloadUrl = assetUrl;
                                        break;
                                    }
                                    else if (assetUrl.find("all-architectures") != std::string::npos && downloadUrl.empty())
                                    {
                                        downloadUrl = assetUrl;  // 备选
                                    }
                                #elif defined(_M_ARM64)
                                    // ARM64 系统
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
                                    // 32位系统优先选择 x86/Win32 版本
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

                break;  // 找到最新的 Stock release 就退出
            }
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
        CInternetSession session(L"Stock Plugin Updater");
        session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 30000);
        session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 30000);

        CHttpFile* pFile = (CHttpFile*)session.OpenURL(url.c_str(), 1,
            INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE,
            NULL, 0);

        if (!pFile)
            return false;

        DWORD dwStatusCode;
        pFile->QueryInfoStatusCode(dwStatusCode);

        if (dwStatusCode != HTTP_STATUS_OK)
        {
            pFile->Close();
            delete pFile;
            return false;
        }

        // 创建本地文件
        CFile localFile;
        if (!localFile.Open(savePath.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
        {
            pFile->Close();
            delete pFile;
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
        pFile->Close();
        delete pFile;
        session.Close();

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
        CInternetSession session(L"Network Check", 1, INTERNET_OPEN_TYPE_PRECONFIG);
        session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 3000);
        session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 3000);

        CHttpFile* pFile = (CHttpFile*)session.OpenURL(
            L"https://api.github.com",
            1,
            INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT,
            NULL, 0);

        if (pFile)
        {
            DWORD dwStatusCode = 0;
            pFile->QueryInfoStatusCode(dwStatusCode);
            pFile->Close();
            delete pFile;
            session.Close();

            // 任何有效的 HTTP 响应都表示网络可用
            return (dwStatusCode > 0);
        }

        session.Close();
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
