#pragma once

#include <string>
#include <Windows.h>

namespace StockPlugin {
namespace Core {

/// @brief 资源提供接口
/// @details 抽象资源（字符串、图标）的获取，支持依赖注入和单元测试
class IResourceProvider
{
public:
    virtual ~IResourceProvider() = default;

    /// @brief 获取字符串资源
    /// @param id 资源 ID
    /// @return 字符串引用
    virtual const std::wstring& GetString(UINT id) = 0;

    /// @brief 获取图标资源
    /// @param id 资源 ID
    /// @param size 图标大小（默认 16）
    /// @return 图标句柄
    virtual HICON GetIcon(UINT id, int size = 16) = 0;

    /// @brief 重置字符串缓存
    virtual void ResetStringCache() = 0;

    /// @brief 设置 DPI 值
    /// @param dpi DPI 值
    virtual void SetDPI(int dpi) = 0;

    /// @brief 获取 DPI 值
    /// @return DPI 值
    virtual int GetDPI() const = 0;

    /// @brief DPI 缩放
    /// @param pixel 原始像素值
    /// @return 缩放后的像素值
    virtual int ScaleDPI(int pixel) const = 0;

    /// @brief DPI 缩放（浮点）
    virtual float ScaleDPIF(float pixel) const = 0;

    /// @brief 反向 DPI 缩放
    virtual int ReverseDPI(int pixel) const = 0;
};

} // namespace Core
} // namespace StockPlugin
