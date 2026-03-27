#include "pch.h"
#include "StockItem.h"
#include "StockConstants.h"
#include "DataManager.h"
#include "Stock.h"
#include "Common.h"
#include <algorithm>
#include <cmath>
#include "FloatingWnd.h"
#undef min
#undef max

namespace
{
CString ToCStringOrFallback(const std::wstring& value, const CString& fallback)
{
    return value.empty() ? fallback : CString(value.c_str());
}

CString ToCStringOrFallback(const std::wstring& value, const wchar_t* fallback)
{
    return ToCStringOrFallback(value, CString(fallback));
}

int GetMeasuredPriceWidth(CDC* pDC, const std::shared_ptr<STOCK::StockData>& data)
{
    return pDC->GetTextExtent(ToCStringOrFallback(data ? data->realTimeData.displayPrice : std::wstring{}, L"999999.99999999")).cx;
}

int GetMeasuredFluctuationWidth(CDC* pDC, const std::shared_ptr<STOCK::StockData>& data)
{
    CString fallback(L"+999.99%");
    return (std::max)(pDC->GetTextExtent(fallback).cx,
        pDC->GetTextExtent(ToCStringOrFallback(data ? data->realTimeData.displayFluctuation : std::wstring{}, fallback)).cx);
}

CString GetStatusAwareName(const std::shared_ptr<STOCK::StockData>& data, const std::wstring& fallbackCode)
{
    if (data != nullptr)
    {
        std::wstring name = data->GetDisplayNameWithStatus();
        if (!name.empty())
            return CString(name.c_str());
    }
    return CString(fallbackCode.c_str());
}

bool TryGetChangePercent(const std::shared_ptr<STOCK::StockData>& data, double& outChangePercent)
{
    if (data == nullptr)
        return false;

    outChangePercent = data->realTimeData.GetChangePercent();
    return std::isfinite(outChangePercent);
}
}

const wchar_t *StockItem::GetItemName() const
{
    std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());

    const std::wstring currentStockId = GetPrimaryStockId();

    if (currentStockId.empty())
    {
        m_cached_item_name = g_data.StringRes(IDS_PLUGIN_ITEM_NAME).GetString();
        m_cached_item_name += std::to_wstring(index);
        return m_cached_item_name.c_str();
    }

    auto data = g_data.GetStockData(currentStockId);
    if (data == nullptr)
    {
        m_cached_item_name = g_data.StringRes(IDS_PLUGIN_ITEM_NAME).GetString();
        m_cached_item_name += std::to_wstring(index);
        return m_cached_item_name.c_str();
    }

    if (!data->info.is_ok)
    {
        // 加载失败
        m_cached_item_name = currentStockId + L" " + g_data.StringRes(IDS_LOAD_FAIL).GetString();
    }
    else
    {
        // 使用带状态前缀的智能简称或别名
        m_cached_item_name = data->GetDisplayNameWithStatus();
        if (m_cached_item_name.empty())
        {
            m_cached_item_name = g_data.StringRes(IDS_PLUGIN_ITEM_NAME).GetString();
            m_cached_item_name += std::to_wstring(index);
        }
    }
    return m_cached_item_name.c_str();
}

const wchar_t *StockItem::GetItemId() const
{
    m_cached_item_id = L"qL0KmmYi";
    m_cached_item_id += std::to_wstring(index);
    return m_cached_item_id.c_str();
}

const wchar_t *StockItem::GetItemLableText() const
{
    return L"";
}

const wchar_t *StockItem::GetItemValueText() const
{
    return L"";
}

bool StockItem::IsCustomDraw() const
{
    return true;
}
int StockItem::GetItemWidthEx(void *hDC) const
{
    if (hDC == nullptr)
        return 0;
    CDC *pDC = CDC::FromHandle((HDC)hDC);
    if (pDC == nullptr)
        return 0;

    std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());

    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;

    int itemSpacing = pDC->GetTextExtent(_T("  ")).cx;

    if (codes.size() >= 2)
    {
        if (settings.displayMode == StockDisplayMode::ShowAll)
        {
            size_t total = codes.size();
            size_t numCols = (total + 1) / 2;

            int totalWidth = 0;
            for (size_t col = 0; col < numCols; col++)
            {
                size_t row1Idx = col * 2;
                size_t row2Idx = col * 2 + 1;

                int col1Width = (row1Idx < total) ? CalcStockDisplayWidth(pDC, codes[row1Idx]) : 0;
                int col2Width = (row2Idx < total) ? CalcStockDisplayWidth(pDC, codes[row2Idx]) : 0;
                int colWidth = (std::max)(col1Width, col2Width);

                if (totalWidth > 0)
                    totalWidth += itemSpacing;
                totalWidth += colWidth;
            }
            return totalWidth;
        }
        else
        {
            size_t firstIdx = Stock::Instance().GetCurrentDisplayIndex();
            size_t secondIdx = Stock::Instance().GetSecondRowIndex();

            int row1Width = (firstIdx < codes.size()) ? CalcStockDisplayWidth(pDC, codes[firstIdx]) : 0;
            int row2Width = (secondIdx < codes.size()) ? CalcStockDisplayWidth(pDC, codes[secondIdx]) : 0;

            return (std::max)(row1Width, row2Width);
        }
    }

    if (!codes.empty())
    {
        return CalcStockDisplayWidth(pDC, codes[0]);
    }

    return 0;
}

void StockItem::DrawItem(void *hDC, int x, int y, int w, int h, bool dark_mode)
{
    if (hDC == nullptr)
        return;
    CDC *pDC = CDC::FromHandle((HDC)hDC);
    if (pDC == nullptr)
        return;

    std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());

    // 文本颜色
    COLORREF color_default = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    COLORREF color_red = dark_mode ? RGB(255, 121, 120) : StockConstants::COLOR_RISE;
    COLORREF color_green = dark_mode ? RGB(111, 215, 149) : StockConstants::COLOR_FALL;

    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;

    // 2个以上股票时，分2行绘制（2xN矩阵）
    if (codes.size() >= 2)
    {
        DrawMultiRow(pDC, x, y, w, h, color_default, color_red, color_green);
        return;
    }

    // 单个股票绘制
    std::wstring currentStockId = GetPrimaryStockId();
    if (currentStockId.empty())
        return;

    DrawStockRow(pDC, currentStockId, x, y, w, h, color_default, color_red, color_green);
}

const wchar_t *StockItem::GetItemValueSampleText() const
{
    return L"";
}

int StockItem::OnMouseEvent(MouseEventType type, int x, int y, void *hWnd, int flag)
{
    CWnd *pWnd = CWnd::FromHandle((HWND)hWnd);
    TRACE(L"OnMouseEvent: %d\n", type);

    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;

    CRect itemRect;
    if (pWnd)
        pWnd->GetClientRect(&itemRect);
    int availableWidth = itemRect.Width();

    switch (type)
    {
    case IPluginItem::MT_RCLICKED:
        Stock::Instance().ShowContextMenu(pWnd);
        return 1;

    case IPluginItem::MT_LCLICKED:
    {
        if (settings.displayMode != StockDisplayMode::ShowAll)
        {
            std::wstring clickedStockId = GetPrimaryStockId();
            if (clickedStockId.find(kSZ) == 0 || clickedStockId.find(kBJ) == 0 || clickedStockId.find(kSH) == 0)
            {
                Stock::Instance().ShowFloatingWnd(hWnd, CPoint(x, y), clickedStockId);
                return 1;
            }
            return 0;
        }

        std::wstring clickedStockId;
        if (!HitTestShowAllStock(pWnd, x, y, availableWidth, clickedStockId))
        {
            return 0;
        }

        if (clickedStockId.find(kSZ) == 0 || clickedStockId.find(kBJ) == 0 || clickedStockId.find(kSH) == 0)
        {
            Stock::Instance().ShowFloatingWnd(hWnd, CPoint(x, y), clickedStockId);
            return 1;
        }
        MessageBox((HWND)hWnd, g_data.StringRes(IDS_UNSUPPORT_SHOW_KLINE_STOCK_TIP), g_data.StringRes(IDS_PLUGIN_NAME), MB_ICONINFORMATION | MB_OK);
        break;
    }

    case IPluginItem::MT_WHEEL_UP:
    case IPluginItem::MT_WHEEL_DOWN:
    {
        if (settings.displayMode != StockDisplayMode::ShowAll && codes.size() > 1)
        {
            if (type == IPluginItem::MT_WHEEL_DOWN)
            {
                Stock::Instance().SwitchToNextStock();
            }
            else
            {
                for (size_t i = 0; i < codes.size() - 1; ++i)
                    Stock::Instance().SwitchToNextStock();
            }
            return 1;
        }
        break;
    }

    case IPluginItem::MT_DBCLICKED:
    {
        // 非ShowAll模式下，双击切换到下一只股票
        if (settings.displayMode != StockDisplayMode::ShowAll)
        {
            Stock::Instance().SwitchToNextStock();
            return 1;
        }
        // ShowAll模式下双击不做处理
        break;
    }

    default:
        break;
    }
    return 0;
}

int StockItem::OnKeboardEvent(int key, bool ctrl, bool shift, bool alt, void* hWnd, int flag)
{
    UNREFERENCED_PARAMETER(ctrl);
    UNREFERENCED_PARAMETER(shift);
    UNREFERENCED_PARAMETER(alt);
    UNREFERENCED_PARAMETER(flag);

    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;
    if (codes.empty())
        return 0;

    if (settings.displayMode != StockDisplayMode::ShowAll && codes.size() > 1)
    {
        if (key == VK_LEFT || key == VK_UP)
        {
            Stock::Instance().SwitchToPreviousStock();
            return 1;
        }
        if (key == VK_RIGHT || key == VK_DOWN || key == VK_SPACE)
        {
            Stock::Instance().SwitchToNextStock();
            return 1;
        }
    }

    if (key == VK_RETURN && (settings.displayMode != StockDisplayMode::ShowAll || codes.size() == 1))
    {
        std::wstring currentStockId = GetPrimaryStockId();
        if (currentStockId.find(kSZ) == 0 || currentStockId.find(kBJ) == 0 || currentStockId.find(kSH) == 0)
        {
            CWnd* pWnd = CWnd::FromHandle((HWND)hWnd);
            CRect rect{};
            if (pWnd != nullptr)
            {
                pWnd->GetClientRect(&rect);
            }
            Stock::Instance().ShowFloatingWnd(hWnd, rect.CenterPoint(), currentStockId);
            return 1;
        }
    }

    return 0;
}

int StockItem::CalcStockDisplayWidth(CDC *pDC, const std::wstring& code)
{
    auto data = g_data.GetStockData(code);
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();

    const int space_width = pDC->GetTextExtent(_T(" ")).cx;
    const int price_width = GetMeasuredPriceWidth(pDC, data);
    const int fluctuation_width = GetMeasuredFluctuationWidth(pDC, data);

    int width = price_width + space_width + fluctuation_width;

    if (settings.showStockName)
    {
        if (data != nullptr)
        {
            CString stock_name{GetStatusAwareName(data, code)};
            stock_name += _T(": ");
            width += pDC->GetTextExtent(stock_name).cx;
        }
        else
        {
            CString stock_name{code.c_str()};
            stock_name += _T(": ");
            width += pDC->GetTextExtent(stock_name).cx;
        }
    }
    return width;
}

void StockItem::DrawStockRow(CDC *pDC, const std::wstring& code, int x, int y, int w, int h,
    COLORREF color_default, COLORREF color_red, COLORREF color_green)
{
    auto data = g_data.GetStockData(code);
    if (data == nullptr)
        return;

    SettingsSnapshot settings = g_data.GetSettingsSnapshot();

    CRect rect(CPoint(x, y), CSize(w, h));

    CString priceText = ToCStringOrFallback(data->realTimeData.displayPrice, L"--");
    CString fluctText = ToCStringOrFallback(data->realTimeData.displayFluctuation, L"--%");

    int fluctuation_width = GetMeasuredFluctuationWidth(pDC, data);
    int price_width = GetMeasuredPriceWidth(pDC, data);
    int space_width = pDC->GetTextExtent(_T(" ")).cx;

    CRect rect_fluctuation{rect};
    rect_fluctuation.left = rect.right - fluctuation_width;

    CRect rect_price{rect};
    rect_price.right = rect_fluctuation.left - space_width;
    rect_price.left = rect_price.right - price_width;

    // 绘制名称
    if (settings.showStockName)
    {
        pDC->SetTextColor(color_default);
        CString stock_name{GetStatusAwareName(data, code)};
        stock_name += _T(": ");
        CRect rect_name{rect};
        rect_name.right = rect_price.left;
        pDC->DrawText(stock_name, rect_name, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);
    }

    // 设置数值颜色
    if (settings.colorWithPrice)
    {
        double changePercent = 0.0;
        if (TryGetChangePercent(data, changePercent))
        {
            if (changePercent < -0.0001)
                pDC->SetTextColor(color_green);
            else if (changePercent > 0.0001)
                pDC->SetTextColor(color_red);
            else
                pDC->SetTextColor(color_default);
        }
        else
        {
            pDC->SetTextColor(color_default);
        }
    }
    else
    {
        pDC->SetTextColor(color_default);
    }

    // 绘制价格
    pDC->DrawText(priceText, rect_price, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);

    // 绘制涨跌幅
    pDC->DrawText(fluctText, rect_fluctuation, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);
}

void StockItem::DrawMultiRow(CDC *pDC, int x, int y, int w, int h,
    COLORREF color_default, COLORREF color_red, COLORREF color_green)
{
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;
    size_t total = codes.size();
    if (total < 2)
        return;

    int rowHeight = h / 2;
    int itemSpacing = pDC->GetTextExtent(_T("  ")).cx;
    if (settings.displayMode == StockDisplayMode::ShowAll)
    {
        size_t numCols = (total + 1) / 2;

        std::vector<int> colWidths(numCols);
        for (size_t col = 0; col < numCols; col++)
        {
            size_t row1Idx = col * 2;
            size_t row2Idx = col * 2 + 1;

            int col1Width = (row1Idx < total) ? CalcStockDisplayWidth(pDC, codes[row1Idx]) : 0;
            int col2Width = (row2Idx < total) ? CalcStockDisplayWidth(pDC, codes[row2Idx]) : 0;
            colWidths[col] = (std::max)(col1Width, col2Width);
        }

        int currentX = x + w;
        for (int col = static_cast<int>(numCols) - 1; col >= 0; col--)
        {
            int colWidth = colWidths[col];
            currentX -= colWidth;

            size_t row1Idx = col * 2;
            size_t row2Idx = col * 2 + 1;

            if (row1Idx < total)
            {
                DrawStockRow(pDC, codes[row1Idx], currentX, y, colWidth, rowHeight,
                    color_default, color_red, color_green);
            }

            if (row2Idx < total)
            {
                DrawStockRow(pDC, codes[row2Idx], currentX, y + rowHeight, colWidth, rowHeight,
                    color_default, color_red, color_green);
            }

            currentX -= itemSpacing;
        }
    }
    else
    {
        size_t firstIdx = Stock::Instance().GetCurrentDisplayIndex();
        size_t secondIdx = Stock::Instance().GetSecondRowIndex();

        std::wstring firstCode = (firstIdx < total) ? codes[firstIdx] : codes[0];
        std::wstring secondCode = (secondIdx < total) ? codes[secondIdx] : codes[0];

        DrawStockRow(pDC, firstCode, x, y, w, rowHeight,
            color_default, color_red, color_green);

        DrawStockRow(pDC, secondCode, x, y + rowHeight, w, rowHeight,
            color_default, color_red, color_green);
    }
}

std::wstring StockItem::GetPrimaryStockId() const
{
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;

    if (codes.empty())
        return std::wstring();

    if (codes.size() == 1 || settings.displayMode == StockDisplayMode::ShowAll)
        return codes.front();

    size_t currentIndex = Stock::Instance().GetCurrentDisplayIndex();
    if (currentIndex >= codes.size())
        currentIndex = 0;
    return codes[currentIndex];
}

bool StockItem::HitTestShowAllStock(CWnd* pWnd, int clickX, int clickY, int width, std::wstring& clickedStockId) const
{
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();
    const auto& codes = settings.stockCodes;
    if (pWnd == nullptr || codes.size() < 2)
        return false;

    CClientDC dc(pWnd);
    CFont* pFont = pWnd->GetFont();
    CFont* pOldFont = (pFont != nullptr) ? dc.SelectObject(pFont) : nullptr;

    const size_t total = codes.size();
    const size_t numCols = (total + 1) / 2;
    const int itemSpacing = dc.GetTextExtent(_T("  ")).cx;

    std::vector<int> colWidths(numCols);
    int totalWidth = 0;
    for (size_t col = 0; col < numCols; ++col)
    {
        size_t row1Idx = col * 2;
        size_t row2Idx = col * 2 + 1;
        int col1Width = (row1Idx < total) ? CalcStockDisplayWidth(&dc, codes[row1Idx]) : 0;
        int col2Width = (row2Idx < total) ? CalcStockDisplayWidth(&dc, codes[row2Idx]) : 0;
        colWidths[col] = (std::max)(col1Width, col2Width);
        totalWidth += colWidths[col];
        if (col > 0)
            totalWidth += itemSpacing;
    }

    CRect clientRect{};
    pWnd->GetClientRect(&clientRect);
    const int rowSplitY = (std::max)(1, clientRect.Height() / 2);
    const bool isSecondRow = (clickY >= rowSplitY);

    const int availableWidth = width > 0 ? width : clientRect.Width();
    const int startX = (std::max)(0, availableWidth - totalWidth);
    int currentX = startX + totalWidth;
    for (int col = static_cast<int>(numCols) - 1; col >= 0; --col)
    {
        currentX -= colWidths[col];
        if (clickX >= currentX && clickX < currentX + colWidths[col])
        {
            size_t index = static_cast<size_t>(col) * 2 + (isSecondRow ? 1 : 0);
            if (index < total)
            {
                clickedStockId = codes[index];
                if (pOldFont != nullptr)
                    dc.SelectObject(pOldFont);
                return true;
            }
            break;
        }
        currentX -= itemSpacing;
    }

    if (pOldFont != nullptr)
        dc.SelectObject(pOldFont);
    return false;
}
