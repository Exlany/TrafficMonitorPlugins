#include "pch.h"
#include "CStockItemRenderer.h"
#include "../StockDef.h"
#include "../StockConstants.h"
#include <algorithm>

#undef min
#undef max

namespace StockPlugin {
namespace Presentation {

CStockItemRenderer::CStockItemRenderer() = default;

CStockItemRenderer::~CStockItemRenderer() = default;

void CStockItemRenderer::SetDarkMode(bool dark)
{
    m_darkMode = dark;
}

void CStockItemRenderer::SetShowStockName(bool show)
{
    m_showStockName = show;
}

void CStockItemRenderer::SetColorWithPrice(bool color)
{
    m_colorWithPrice = color;
}

COLORREF CStockItemRenderer::GetDefaultColor() const
{
    return m_darkMode ? RGB(255, 255, 255) : RGB(0, 0, 0);
}

COLORREF CStockItemRenderer::GetRiseColor() const
{
    return m_darkMode ? RGB(255, 121, 120) : StockConstants::COLOR_RISE;
}

COLORREF CStockItemRenderer::GetFallColor() const
{
    return m_darkMode ? RGB(111, 215, 149) : StockConstants::COLOR_FALL;
}

void CStockItemRenderer::DrawSingleStock(HDC hDC, const STOCK::StockData& data, int x, int y, int w, int h)
{
    DrawStockRowInternal(hDC, data, x, y, w, h);
}

void CStockItemRenderer::DrawStockRow(HDC hDC, const std::wstring& code, int x, int y, int w, int h,
                                       std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> dataGetter)
{
    if (!dataGetter)
        return;

    auto data = dataGetter(code);
    if (!data)
        return;

    DrawStockRowInternal(hDC, *data, x, y, w, h);
}

void CStockItemRenderer::DrawStockRowInternal(HDC hDC, const STOCK::StockData& data, int x, int y, int w, int h)
{
    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return;

    CRect rect(CPoint(x, y), CSize(w, h));

    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;
    int price_width = pDC->GetTextExtent(_T("99999.999")).cx;
    int space_width = pDC->GetTextExtent(_T(" ")).cx;

    CRect rect_fluctuation{rect};
    rect_fluctuation.left = rect.right - fluctuation_width;

    CRect rect_price{rect};
    rect_price.right = rect_fluctuation.left - space_width;
    rect_price.left = rect_price.right - price_width;

    COLORREF color_default = GetDefaultColor();
    COLORREF color_red = GetRiseColor();
    COLORREF color_green = GetFallColor();

    // 绘制名称
    if (data.info.is_ok && m_showStockName)
    {
        pDC->SetTextColor(color_default);
        CString stock_name{data.GetDisplayName().c_str()};
        stock_name += _T(": ");
        CRect rect_name{rect};
        rect_name.right = rect_price.left;
        pDC->DrawText(stock_name, rect_name, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);
    }

    // 设置数值颜色
    if (m_colorWithPrice)
    {
        if (data.realTimeData.displayFluctuation.find('-') != std::wstring::npos)
            pDC->SetTextColor(color_green);
        else
            pDC->SetTextColor(color_red);
    }
    else
    {
        pDC->SetTextColor(color_default);
    }

    // 绘制价格
    const wchar_t* priceText = data.realTimeData.displayPrice.empty()
        ? L"--" : data.realTimeData.displayPrice.c_str();
    pDC->DrawText(priceText, rect_price, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);

    // 绘制涨跌幅
    const wchar_t* fluctText = data.realTimeData.displayFluctuation.empty()
        ? L"--" : data.realTimeData.displayFluctuation.c_str();
    pDC->DrawText(fluctText, rect_fluctuation, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);
}

void CStockItemRenderer::DrawMultiRow(HDC hDC, const std::vector<std::wstring>& codes, int x, int y, int w, int h,
                                       bool showAll, size_t currentIndex, size_t secondIndex,
                                       std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> dataGetter)
{
    if (!dataGetter || codes.size() < 2)
        return;

    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return;

    size_t total = codes.size();
    int rowHeight = h / 2;
    int itemSpacing = pDC->GetTextExtent(_T("  ")).cx;
    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;
    int space_width = pDC->GetTextExtent(_T(" ")).cx;
    int price_width = pDC->GetTextExtent(_T("99999.999")).cx;

    if (showAll)
    {
        // ShowAll 模式：2xN 矩阵
        size_t numCols = (total + 1) / 2;

        // 计算每列宽度
        std::vector<int> colWidths(numCols);
        for (size_t col = 0; col < numCols; col++)
        {
            size_t row1Idx = col * 2;
            size_t row2Idx = col * 2 + 1;

            int col1Width = (row1Idx < total) ? CalcStockDisplayWidth(hDC, codes[row1Idx], dataGetter) : 0;
            int col2Width = (row2Idx < total) ? CalcStockDisplayWidth(hDC, codes[row2Idx], dataGetter) : 0;
            colWidths[col] = std::max(col1Width, col2Width);
        }

        // 从右向左绘制
        int currentX = x + w;
        for (int col = static_cast<int>(numCols) - 1; col >= 0; col--)
        {
            int colWidth = colWidths[col];
            currentX -= colWidth;

            size_t row1Idx = col * 2;
            size_t row2Idx = col * 2 + 1;

            if (row1Idx < total)
            {
                DrawStockRow(hDC, codes[row1Idx], currentX, y, colWidth, rowHeight, dataGetter);
            }

            if (row2Idx < total)
            {
                DrawStockRow(hDC, codes[row2Idx], currentX, y + rowHeight, colWidth, rowHeight, dataGetter);
            }

            currentX -= itemSpacing;
        }
    }
    else
    {
        // 轮播/手动/智能模式：显示当前和下一个
        std::wstring firstCode = (currentIndex < total) ? codes[currentIndex] : codes[0];
        std::wstring secondCode = (secondIndex < total) ? codes[secondIndex] : codes[0];

        DrawStockRow(hDC, firstCode, x, y, w, rowHeight, dataGetter);
        DrawStockRow(hDC, secondCode, x, y + rowHeight, w, rowHeight, dataGetter);
    }
}

int CStockItemRenderer::CalcStockDisplayWidth(HDC hDC, const std::wstring& code,
                                               std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> dataGetter)
{
    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return 0;

    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;
    int space_width = pDC->GetTextExtent(_T(" ")).cx;
    int price_width = pDC->GetTextExtent(_T("99999.999")).cx;

    int width = price_width + space_width + fluctuation_width;

    if (m_showStockName && dataGetter)
    {
        auto data = dataGetter(code);
        if (data && !data->GetDisplayName().empty())
        {
            CString stock_name{data->GetDisplayName().c_str()};
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

int CStockItemRenderer::CalcTotalWidth(HDC hDC, const std::vector<std::wstring>& codes,
                                        bool showAll, size_t currentIndex, size_t secondIndex,
                                        std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> dataGetter)
{
    if (codes.empty())
        return 0;

    CDC* pDC = CDC::FromHandle(hDC);
    if (!pDC)
        return 0;

    int itemSpacing = pDC->GetTextExtent(_T("  ")).cx;

    if (codes.size() >= 2)
    {
        if (showAll)
        {
            // ShowAll 模式：计算所有列的总宽度
            size_t total = codes.size();
            size_t numCols = (total + 1) / 2;

            int totalWidth = 0;
            for (size_t col = 0; col < numCols; col++)
            {
                size_t row1Idx = col * 2;
                size_t row2Idx = col * 2 + 1;

                int col1Width = (row1Idx < total) ? CalcStockDisplayWidth(hDC, codes[row1Idx], dataGetter) : 0;
                int col2Width = (row2Idx < total) ? CalcStockDisplayWidth(hDC, codes[row2Idx], dataGetter) : 0;
                int colWidth = std::max(col1Width, col2Width);

                if (totalWidth > 0)
                    totalWidth += itemSpacing;
                totalWidth += colWidth;
            }
            return totalWidth;
        }
        else
        {
            // 轮播/手动/智能模式：取两行中较宽的
            int row1Width = (currentIndex < codes.size()) ? CalcStockDisplayWidth(hDC, codes[currentIndex], dataGetter) : 0;
            int row2Width = (secondIndex < codes.size()) ? CalcStockDisplayWidth(hDC, codes[secondIndex], dataGetter) : 0;
            return std::max(row1Width, row2Width);
        }
    }

    // 单个股票
    return CalcStockDisplayWidth(hDC, codes[0], dataGetter);
}

} // namespace Presentation
} // namespace StockPlugin
