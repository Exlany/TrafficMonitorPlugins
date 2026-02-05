#pragma once
#include "PluginInterface.h"
#include "FloatingWnd.h"

class StockItem : public IPluginItem
{
public:
    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;
    virtual int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) override;
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;
	virtual bool IsCustomDraw() const override;
	virtual int GetItemWidthEx(void* hDC) const override;

    int index;
    std::wstring stock_id;
    bool enable;

    // 缓存的显示文本（避免使用static变量导致多item时互相覆盖）
    mutable std::wstring m_cached_item_id;
    mutable std::wstring m_cached_item_name;

private:
    // 绘制辅助函数
    void DrawSingleStock(CDC *pDC, const std::wstring& code, int x, int y, int w, int h,
        COLORREF color_default, COLORREF color_red, COLORREF color_green);
    void DrawMultiRow(CDC *pDC, int x, int y, int w, int h,
        COLORREF color_default, COLORREF color_red, COLORREF color_green);
    void DrawSingleStockInRow(CDC *pDC, const std::wstring& code, int x, int y, int w, int h,
        COLORREF color_default, COLORREF color_red, COLORREF color_green, int fixedNameWidth = 0);
};
