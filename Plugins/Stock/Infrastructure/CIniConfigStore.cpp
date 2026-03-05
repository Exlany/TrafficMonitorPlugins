#include "pch.h"
#include "CIniConfigStore.h"
#include "../../utilities/IniHelper.h"

namespace StockPlugin {
namespace Infrastructure {

CIniConfigStore::CIniConfigStore() = default;

CIniConfigStore::~CIniConfigStore() = default;

void CIniConfigStore::Load(const std::wstring& path)
{
    m_path = path;
    m_iniHelper = std::make_unique<utilities::CIniHelper>(path);
}

void CIniConfigStore::Save()
{
    if (m_iniHelper)
    {
        m_iniHelper->Save();
    }
}

int CIniConfigStore::GetInt(const wchar_t* section, const wchar_t* key, int defaultValue)
{
    if (!m_iniHelper) return defaultValue;
    return m_iniHelper->GetInt(section, key, defaultValue);
}

bool CIniConfigStore::GetBool(const wchar_t* section, const wchar_t* key, bool defaultValue)
{
    if (!m_iniHelper) return defaultValue;
    return m_iniHelper->GetBool(section, key, defaultValue);
}

std::wstring CIniConfigStore::GetString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue)
{
    if (!m_iniHelper) return defaultValue;
    return m_iniHelper->GetString(section, key, defaultValue);
}

void CIniConfigStore::SetInt(const wchar_t* section, const wchar_t* key, int value)
{
    if (m_iniHelper)
    {
        m_iniHelper->WriteInt(section, key, value);
    }
}

void CIniConfigStore::SetBool(const wchar_t* section, const wchar_t* key, bool value)
{
    if (m_iniHelper)
    {
        m_iniHelper->WriteBool(section, key, value);
    }
}

void CIniConfigStore::SetString(const wchar_t* section, const wchar_t* key, const wchar_t* value)
{
    if (m_iniHelper)
    {
        m_iniHelper->WriteString(section, key, value);
    }
}

const std::wstring& CIniConfigStore::GetPath() const
{
    return m_path;
}

void CIniConfigStore::GetStringList(const wchar_t* section, const wchar_t* key,
                                     std::vector<std::wstring>& values,
                                     const std::vector<std::wstring>& defaultValue)
{
    if (m_iniHelper)
    {
        m_iniHelper->GetStringList(section, key, values, defaultValue);
    }
    else
    {
        values = defaultValue;
    }
}

void CIniConfigStore::SetStringList(const wchar_t* section, const wchar_t* key,
                                     const std::vector<std::wstring>& values)
{
    if (m_iniHelper)
    {
        m_iniHelper->WriteStringList(section, key, values);
    }
}

} // namespace Infrastructure
} // namespace StockPlugin
