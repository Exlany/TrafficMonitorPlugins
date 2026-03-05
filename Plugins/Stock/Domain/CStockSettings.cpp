#include "pch.h"
#include "CStockSettings.h"
#include "../Infrastructure/CIniConfigStore.h"

namespace StockPlugin {
namespace Domain {

CStockSettings::CStockSettings() = default;

CStockSettings::~CStockSettings() = default;

void CStockSettings::Load(Core::IConfigStore& store)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    // 尝试转换为 CIniConfigStore 以使用扩展方法
    auto* iniStore = dynamic_cast<Infrastructure::CIniConfigStore*>(&store);

    if (iniStore)
    {
        iniStore->GetStringList(L"config", L"stock_code", m_stockCodes, std::vector<std::wstring>{});

        // 加载别名
        std::vector<std::wstring> aliasList;
        iniStore->GetStringList(L"config", L"stock_aliases", aliasList, std::vector<std::wstring>{});
        m_aliases.clear();
        for (const auto& item : aliasList)
        {
            size_t pos = item.find(L'=');
            if (pos != std::wstring::npos)
            {
                std::wstring code = item.substr(0, pos);
                std::wstring alias = item.substr(pos + 1);
                m_aliases[code] = alias;
            }
        }
    }

    m_fullDay = store.GetBool(L"config", L"full_day", true);
    m_showStockName = store.GetBool(L"config", L"show_stock_name", true);
    m_colorWithPrice = store.GetBool(L"config", L"color_with_price", true);
    m_klineWidth = static_cast<unsigned int>(store.GetInt(L"config", L"kline_width", 450));
    m_klineHeight = static_cast<unsigned int>(store.GetInt(L"config", L"kline_height", 210));
    m_priceDecimal = store.GetInt(L"config", L"price_decimal", 3);
    m_displayMode = static_cast<StockDisplayMode>(store.GetInt(L"config", L"display_mode", 0));
    m_carouselInterval = store.GetInt(L"config", L"carousel_interval", 5);
    m_checkUpdate = store.GetBool(L"config", L"check_update", true);
}

void CStockSettings::Save(Core::IConfigStore& store) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    auto* iniStore = dynamic_cast<Infrastructure::CIniConfigStore*>(&store);

    if (iniStore)
    {
        iniStore->SetStringList(L"config", L"stock_code", m_stockCodes);

        // 保存别名
        std::vector<std::wstring> aliasList;
        for (const auto& pair : m_aliases)
        {
            if (!pair.second.empty())
            {
                aliasList.push_back(pair.first + L"=" + pair.second);
            }
        }
        iniStore->SetStringList(L"config", L"stock_aliases", aliasList);
    }

    store.SetBool(L"config", L"full_day", m_fullDay);
    store.SetBool(L"config", L"show_stock_name", m_showStockName);
    store.SetBool(L"config", L"color_with_price", m_colorWithPrice);
    store.SetInt(L"config", L"kline_width", static_cast<int>(m_klineWidth));
    store.SetInt(L"config", L"kline_height", static_cast<int>(m_klineHeight));
    store.SetInt(L"config", L"price_decimal", m_priceDecimal);
    store.SetInt(L"config", L"display_mode", static_cast<int>(m_displayMode));
    store.SetInt(L"config", L"carousel_interval", m_carouselInterval);
    store.SetBool(L"config", L"check_update", m_checkUpdate);

    store.Save();
}

std::vector<std::wstring> CStockSettings::GetStockCodes() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_stockCodes;
}

void CStockSettings::SetStockCodes(const std::vector<std::wstring>& codes)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_stockCodes = codes;
}

void CStockSettings::AddStockCode(const std::wstring& code)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_stockCodes.push_back(code);
}

void CStockSettings::RemoveStockCode(const std::wstring& code)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_stockCodes.erase(
        std::remove(m_stockCodes.begin(), m_stockCodes.end(), code),
        m_stockCodes.end());
}

size_t CStockSettings::GetStockCount() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_stockCodes.size();
}

std::map<std::wstring, std::wstring> CStockSettings::GetAliases() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_aliases;
}

void CStockSettings::SetAliases(const std::map<std::wstring, std::wstring>& aliases)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_aliases = aliases;
}

std::wstring CStockSettings::GetAlias(const std::wstring& code) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_aliases.find(code);
    return (it != m_aliases.end()) ? it->second : std::wstring();
}

void CStockSettings::SetAlias(const std::wstring& code, const std::wstring& alias)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    if (alias.empty())
    {
        m_aliases.erase(code);
    }
    else
    {
        m_aliases[code] = alias;
    }
}

bool CStockSettings::IsFullDay() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_fullDay;
}

void CStockSettings::SetFullDay(bool value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_fullDay = value;
}

bool CStockSettings::ShowStockName() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_showStockName;
}

void CStockSettings::SetShowStockName(bool value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_showStockName = value;
}

bool CStockSettings::ColorWithPrice() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_colorWithPrice;
}

void CStockSettings::SetColorWithPrice(bool value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_colorWithPrice = value;
}

int CStockSettings::GetPriceDecimal() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_priceDecimal;
}

void CStockSettings::SetPriceDecimal(int value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_priceDecimal = value;
}

unsigned int CStockSettings::GetKLineWidth() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_klineWidth;
}

void CStockSettings::SetKLineWidth(unsigned int value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_klineWidth = value;
}

unsigned int CStockSettings::GetKLineHeight() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_klineHeight;
}

void CStockSettings::SetKLineHeight(unsigned int value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_klineHeight = value;
}

StockDisplayMode CStockSettings::GetDisplayMode() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_displayMode;
}

void CStockSettings::SetDisplayMode(StockDisplayMode mode)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_displayMode = mode;
}

int CStockSettings::GetCarouselInterval() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_carouselInterval;
}

void CStockSettings::SetCarouselInterval(int seconds)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_carouselInterval = seconds;
}

bool CStockSettings::IsCheckUpdateEnabled() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_checkUpdate;
}

void CStockSettings::SetCheckUpdateEnabled(bool value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_checkUpdate = value;
}

SettingsSnapshot CStockSettings::CreateSnapshot() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    SettingsSnapshot snapshot;
    snapshot.stockCodes = m_stockCodes;
    snapshot.aliases = m_aliases;
    snapshot.fullDay = m_fullDay;
    snapshot.showStockName = m_showStockName;
    snapshot.colorWithPrice = m_colorWithPrice;
    snapshot.priceDecimal = m_priceDecimal;
    snapshot.klineWidth = m_klineWidth;
    snapshot.klineHeight = m_klineHeight;
    snapshot.displayMode = m_displayMode;
    snapshot.carouselInterval = m_carouselInterval;
    snapshot.checkUpdate = m_checkUpdate;
    return snapshot;
}

void CStockSettings::ApplySnapshot(const SettingsSnapshot& snapshot)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_stockCodes = snapshot.stockCodes;
    m_aliases = snapshot.aliases;
    m_fullDay = snapshot.fullDay;
    m_showStockName = snapshot.showStockName;
    m_colorWithPrice = snapshot.colorWithPrice;
    m_priceDecimal = snapshot.priceDecimal;
    m_klineWidth = snapshot.klineWidth;
    m_klineHeight = snapshot.klineHeight;
    m_displayMode = snapshot.displayMode;
    m_carouselInterval = snapshot.carouselInterval;
    m_checkUpdate = snapshot.checkUpdate;
}

} // namespace Domain
} // namespace StockPlugin
