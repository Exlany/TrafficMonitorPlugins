#include "pch.h"
#include "StockMappings.h"
#include "StockConstants.h"

namespace StockMappings
{

// ============================================================================
// 数据获取函数 (static local 模式，线程安全初始化)
// ============================================================================

const std::map<std::wstring, std::wstring>& GetNameDirectMap()
{
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
    return directMap;
}

const std::vector<std::wstring>& GetNamePrefixes()
{
    static const std::vector<std::wstring> prefixes = {
            L"中国", L"中华", L"全国", L"国家",
            L"上海", L"深圳", L"北京", L"广州", L"天津", L"重庆",
            L"江苏", L"浙江", L"山东", L"四川", L"湖北", L"湖南",
            L"河南", L"河北", L"福建", L"安徽", L"陕西", L"辽宁",
            L"广东", L"云南", L"贵州", L"山西", L"吉林", L"黑龙江",
            L"江西", L"海南", L"甘肃", L"青海", L"宁夏", L"新疆",
            L"西藏", L"内蒙古", L"广西"
        };
    return prefixes;
}

const std::vector<std::wstring>& GetNameSuffixes()
{
    static const std::vector<std::wstring> suffixes = {
            L"股份有限公司", L"有限责任公司", L"有限公司",
            L"集团股份", L"集团公司", L"集团",
            L"控股股份", L"控股公司", L"控股",
            L"实业", L"企业"
        };
    return suffixes;
}

const std::vector<std::wstring>& GetFundCompanies()
{
    static const std::vector<std::wstring> fundCompanies = {
                L"华夏", L"易方达", L"南方", L"广发", L"博时", L"嘉实", L"富国", L"汇添富",
                L"招商", L"工银", L"华安", L"天弘", L"中欧", L"景顺", L"兴全", L"交银",
                L"鹏华", L"国泰", L"华宝", L"银华", L"诺安", L"大成", L"建信", L"中银",
                L"平安", L"万家", L"长城", L"国投", L"华泰", L"申万", L"海富通", L"长盛"
            };
    return fundCompanies;
}

const std::vector<std::wstring>& GetFundSuffixes()
{
    static const std::vector<std::wstring> fundSuffixes = {
                L"联接A", L"联接C", L"联接", L"ETF联接", L"LOF",
                L"(QDII)", L"QDII", L"增强", L"指数"
            };
    return fundSuffixes;
}

const std::vector<std::pair<std::wstring, std::wstring>>& GetCommodityKeywords()
{
    static const std::vector<std::pair<std::wstring, std::wstring>> commodityKeywords = {
                    {L"黄金", L"黄金"}, {L"白银", L"白银"}, {L"原油", L"原油"},
                    {L"豆粕", L"豆粕"}, {L"有色", L"有色"}, {L"能源", L"能源"},
                    {L"铜", L"铜"}, {L"铝", L"铝"}, {L"锌", L"锌"}
                };
    return commodityKeywords;
}

const std::map<std::wstring, std::wstring>& GetUSIndexMap()
{
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
    return usIndexMap;
}

const std::map<std::wstring, std::wstring>& GetUSStockMap()
{
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
    return usStockMap;
}

const std::map<std::wstring, std::wstring>& GetCryptoMap()
{
    static const std::map<std::wstring, std::wstring> cryptoMap;
    return cryptoMap;
}

const std::map<std::wstring, std::wstring>& GetForexMap()
{
    static const std::map<std::wstring, std::wstring> forexMap;
    return forexMap;
}

// ============================================================================
// 智能识别函数
// ============================================================================

std::wstring SmartShortName(const std::wstring& name)
{
    if (name.empty())
        return name;

    // 1. 常见股票名称直接映射（优先级最高）
    const auto& directMap = GetNameDirectMap();
    auto it = directMap.find(name);
    if (it != directMap.end())
        return it->second;

    std::wstring result = name;

    // 2. 移除常见前缀（地名）
    const auto& prefixes = GetNamePrefixes();
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
    const auto& suffixes = GetNameSuffixes();
    for (const auto& suffix : suffixes)
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
        const auto& fundCompanies = GetFundCompanies();
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
        const auto& fundSuffixList = GetFundSuffixes();
        for (const auto& suffix : fundSuffixList)
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
            const auto& commodityKeywords = GetCommodityKeywords();
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

std::wstring SmartStockCode(const std::wstring& input)
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
    const auto& usIndexMap = GetUSIndexMap();
    auto itIndex = usIndexMap.find(code);
    if (itIndex != usIndexMap.end())
        return itIndex->second;

    // 3. 检查是否是已知的美股代码
    const auto& usStockMap = GetUSStockMap();
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
            // 上证: 6开头(A股主板/科创板)、5开头(ETF/基金/债券回购)、9开头(B股)
            if (first == L'6' || first == L'5' || first == L'9')
                return L"sh" + code;
            // 深证: 0开头(主板)、2开头(B股)、3开头(创业板)、1开头(基金/债券)
            if (first == L'0' || first == L'2' || first == L'3' || first == L'1')
                return L"sz" + code;
            // 北交所: 8开头(新股/精选层)、4开头(老三板/基础层)
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

int GetStockTypeIndex(const std::wstring& code)
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

StockConstants::ProductType GetProductType(const std::wstring& code)
{
    if (code.empty())
        return StockConstants::ProductType::Unknown;

    // 上证
    if (code.find(L"sh") == 0)
    {
        // 判断是否为指数 (sh000xxx)
        if (code.length() >= 8 && code.substr(2, 3) == L"000")
            return StockConstants::ProductType::CN_Index;
        // 判断是否为ETF (sh5xxxxx)
        if (code.length() >= 3 && code[2] == L'5')
            return StockConstants::ProductType::ETF;
        return StockConstants::ProductType::SH_Stock;
    }

    // 深证
    if (code.find(L"sz") == 0)
    {
        // 判断是否为指数 (sz399xxx)
        if (code.length() >= 8 && code.substr(2, 3) == L"399")
            return StockConstants::ProductType::CN_Index;
        // 判断是否为ETF (sz1xxxxx)
        if (code.length() >= 3 && code[2] == L'1')
            return StockConstants::ProductType::ETF;
        return StockConstants::ProductType::SZ_Stock;
    }

    // 北交所
    if (code.find(L"bj") == 0)
        return StockConstants::ProductType::BJ_Stock;

    // 港股
    if (code.find(L"rt_hk") == 0)
        return StockConstants::ProductType::HK_Stock;

    // 美股个股
    if (code.find(L"gb_") == 0)
        return StockConstants::ProductType::US_Stock;

    // 国际指数
    if (code.find(L"int_") == 0)
        return StockConstants::ProductType::US_Index;

    // 期货
    if (code.find(L"hf_") == 0)
        return StockConstants::ProductType::Futures;

    // OKX虚拟货币
    if (code.find(L"okx_") == 0)
        return StockConstants::ProductType::Crypto_OKX;

    // 币安虚拟货币
    if (code.find(L"bn_") == 0)
        return StockConstants::ProductType::Crypto_Binance;

    // 外汇
    if (code.find(L"fx_") == 0)
        return StockConstants::ProductType::Forex;

    return StockConstants::ProductType::Unknown;
}

} // namespace StockMappings
