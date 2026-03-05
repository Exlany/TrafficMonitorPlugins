#pragma once

#include "../Core/IResourceProvider.h"
#include <map>
#include <mutex>
#include <atomic>

namespace StockPlugin {
namespace Infrastructure {

/// @brief 资源缓存实现
/// @details 缓存字符串和图标资源，支持线程安全访问
class CResourceCache : public Core::IResourceProvider
{
public:
    CResourceCache();
    ~CResourceCache() override;

    // IResourceProvider 接口实现
    const std::wstring& GetString(UINT id) override;
    HICON GetIcon(UINT id, int size = 16) override;
    void ResetStringCache() override;
    void SetDPI(int dpi) override;
    int GetDPI() const override;
    int ScaleDPI(int pixel) const override;
    float ScaleDPIF(float pixel) const override;
    int ReverseDPI(int pixel) const override;

    /// @brief 从窗口获取 DPI
    void DPIFromWindow(HWND hWnd);

private:
    mutable std::mutex m_stringMutex;
    mutable std::mutex m_iconMutex;
    std::map<UINT, std::wstring> m_stringTable;
    std::map<UINT, HICON> m_icons;
    std::atomic<int> m_dpi{96};

    static const std::wstring s_emptyString;
};

} // namespace Infrastructure
} // namespace StockPlugin
