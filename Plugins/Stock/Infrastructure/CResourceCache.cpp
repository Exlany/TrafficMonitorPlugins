#include "pch.h"
#include "CResourceCache.h"

namespace StockPlugin {
namespace Infrastructure {

const std::wstring CResourceCache::s_emptyString;

CResourceCache::CResourceCache() = default;

CResourceCache::~CResourceCache()
{
    // 释放图标资源
    std::lock_guard<std::mutex> lock(m_iconMutex);
    for (auto& pair : m_icons)
    {
        if (pair.second)
        {
            DestroyIcon(pair.second);
        }
    }
    m_icons.clear();
}

const std::wstring& CResourceCache::GetString(UINT id)
{
    std::lock_guard<std::mutex> lock(m_stringMutex);

    auto it = m_stringTable.find(id);
    if (it != m_stringTable.end())
    {
        return it->second;
    }

    // 加载字符串资源
    CString str;
    if (str.LoadString(id))
    {
        m_stringTable[id] = str.GetString();
        return m_stringTable[id];
    }

    return s_emptyString;
}

HICON CResourceCache::GetIcon(UINT id, int size)
{
    std::lock_guard<std::mutex> lock(m_iconMutex);

    // 使用 id 和 size 组合作为 key
    UINT key = id | (size << 16);
    auto it = m_icons.find(key);
    if (it != m_icons.end())
    {
        return it->second;
    }

    // 加载图标资源
    int scaledSize = ScaleDPI(size);
    HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(id),
                                    IMAGE_ICON, scaledSize, scaledSize, 0);
    if (hIcon)
    {
        m_icons[key] = hIcon;
    }
    return hIcon;
}

void CResourceCache::ResetStringCache()
{
    std::lock_guard<std::mutex> lock(m_stringMutex);
    m_stringTable.clear();
}

void CResourceCache::SetDPI(int dpi)
{
    m_dpi.store(dpi);
}

int CResourceCache::GetDPI() const
{
    return m_dpi.load();
}

int CResourceCache::ScaleDPI(int pixel) const
{
    return pixel * m_dpi.load() / 96;
}

float CResourceCache::ScaleDPIF(float pixel) const
{
    return pixel * static_cast<float>(m_dpi.load()) / 96.0f;
}

int CResourceCache::ReverseDPI(int pixel) const
{
    return pixel * 96 / m_dpi.load();
}

void CResourceCache::DPIFromWindow(HWND hWnd)
{
    if (hWnd == nullptr)
        return;

    HDC hDC = ::GetDC(hWnd);
    if (hDC)
    {
        int dpi = GetDeviceCaps(hDC, LOGPIXELSY);
        m_dpi.store(dpi);
        ::ReleaseDC(hWnd, hDC);
    }
}

} // namespace Infrastructure
} // namespace StockPlugin
