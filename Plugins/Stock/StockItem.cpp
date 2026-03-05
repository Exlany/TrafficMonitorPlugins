#include "pch.h"
#include "StockItem.h"
#include "StockConstants.h"
#include "DataManager.h"
#include "Stock.h"
#include "Common.h"
#include <algorithm>
#include "FloatingWnd.h"
#undef min
#undef max

const wchar_t *StockItem::GetItemName() const
{
    std::lock_guard<std::mutex> lock(g_data.GetStockDataMutex());

    // 如果 stock_id 为空，返回默认名称
    if (stock_id.empty())
    {
        m_cached_item_name = g_data.StringRes(IDS_PLUGIN_ITEM_NAME).GetString();
        m_cached_item_name += std::to_wstring(index);
        return m_cached_item_name.c_str();
    }

    auto data = g_data.GetStockData(stock_id);
    if (data == nullptr)
    {
        m_cached_item_name = g_data.StringRes(IDS_PLUGIN_ITEM_NAME).GetString();
        m_cached_item_name += std::to_wstring(index);
        return m_cached_item_name.c_str();
    }

    if (!data->info.is_ok)
    {
        // 加载失败
        m_cached_item_name = stock_id + L" " + g_data.StringRes(IDS_LOAD_FAIL).GetString();
    }
    else if (!data->info.displayName.empty())
    {
        // 使用智能简称或别名
        m_cached_item_name = data->GetDisplayName();
    }
    else
    {
        // 数据未加载，显示默认名称
        m_cached_item_name = g_data.StringRes(IDS_PLUGIN_ITEM_NAME).GetString();
        m_cached_item_name += std::to_wstring(index);
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

    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;
    int space_width = pDC->GetTextExtent(_T(" ")).cx;
    int itemSpacing = pDC->GetTextExtent(_T("  ")).cx;
    int price_width = pDC->GetTextExtent(_T("99999.999")).cx;

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

                int col1Width = (row1Idx < total) ? CalcStockDisplayWidth(pDC, codes[row1Idx], price_width, space_width, fluctuation_width) : 0;
                int col2Width = (row2Idx < total) ? CalcStockDisplayWidth(pDC, codes[row2Idx], price_width, space_width, fluctuation_width) : 0;
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

            int row1Width = (firstIdx < codes.size()) ? CalcStockDisplayWidth(pDC, codes[firstIdx], price_width, space_width, fluctuation_width) : 0;
            int row2Width = (secondIdx < codes.size()) ? CalcStockDisplayWidth(pDC, codes[secondIdx], price_width, space_width, fluctuation_width) : 0;

            return (std::max)(row1Width, row2Width);
        }
    }

    if (!codes.empty())
    {
        return CalcStockDisplayWidth(pDC, codes[0], price_width, space_width, fluctuation_width);
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
    std::wstring currentStockId = stock_id;
    if (currentStockId.empty() && !codes.empty())
        currentStockId = codes[0];

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

    switch (type)
    {
    case IPluginItem::MT_RCLICKED:
        Stock::Instance().ShowContextMenu(pWnd);
        return 1;

    case IPluginItem::MT_LCLICKED:
    {
        // 非ShowAll模式下，单击不触发走势图（避免与双击切换冲突）
        if (settings.displayMode != StockDisplayMode::ShowAll)
        {
            return 0;  // 不处理单击，让系统继续等待可能的双击
        }

        // ShowAll模式下，单击显示走势图
        std::wstring clickedStockId = stock_id;

        // 多行布局时，根据y坐标判断点击的行
        if (codes.size() >= 2)
        {
            // 获取窗口客户区高度来判断点击的是哪一行
            CRect rect;
            pWnd->GetClientRect(&rect);
            int rowHeight = rect.Height() / 2;

            // 判断点击的是第一行还是第二行
            bool isSecondRow = (y > rowHeight);

            // ShowAll模式：2xN矩阵，第一行是0,2,4...，第二行是1,3,5...
            if (isSecondRow && codes.size() > 1)
            {
                // 第二行：找第一个A股（索引1,3,5...）
                for (size_t i = 1; i < codes.size(); i += 2)
                {
                    if (codes[i].find(kSZ) == 0 || codes[i].find(kBJ) == 0 || codes[i].find(kSH) == 0)
                    {
                        clickedStockId = codes[i];
                        break;
                    }
                }
            }
            else
            {
                // 第一行：找第一个A股（索引0,2,4...）
                for (size_t i = 0; i < codes.size(); i += 2)
                {
                    if (codes[i].find(kSZ) == 0 || codes[i].find(kBJ) == 0 || codes[i].find(kSH) == 0)
                    {
                        clickedStockId = codes[i];
                        break;
                    }
                }
            }
        }

        // 显示走势图（仅支持A股）
        if (clickedStockId.find(kSZ) == 0 || clickedStockId.find(kBJ) == 0 || clickedStockId.find(kSH) == 0)
        {
            CPoint ptScreen = CPoint(x, y);
            Stock::Instance().ShowFloatingWnd(hWnd, ptScreen, clickedStockId);
            return 1;
        }
        else
        {
            MessageBox((HWND)hWnd, g_data.StringRes(IDS_UNSUPPORT_SHOW_KLINE_STOCK_TIP), g_data.StringRes(IDS_PLUGIN_NAME), MB_ICONINFORMATION | MB_OK);
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

int StockItem::CalcStockDisplayWidth(CDC *pDC, const std::wstring& code, int price_width, int space_width, int fluctuation_width)
{
    auto data = g_data.GetStockData(code);
    SettingsSnapshot settings = g_data.GetSettingsSnapshot();

    int width = price_width + space_width + fluctuation_width;

    if (settings.showStockName)
    {
        if (data != nullptr && !data->GetDisplayName().empty())
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

void StockItem::DrawStockRow(CDC *pDC, const std::wstring& code, int x, int y, int w, int h,
    COLORREF color_default, COLORREF color_red, COLORREF color_green)
{
    auto data = g_data.GetStockData(code);
    if (data == nullptr)
        return;

    SettingsSnapshot settings = g_data.GetSettingsSnapshot();

    CRect rect(CPoint(x, y), CSize(w, h));

    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;
    int price_width = pDC->GetTextExtent(_T("99999.999")).cx;
    int space_width = pDC->GetTextExtent(_T(" ")).cx;

    CRect rect_fluctuation{rect};
    rect_fluctuation.left = rect.right - fluctuation_width;

    CRect rect_price{rect};
    rect_price.right = rect_fluctuation.left - space_width;
    rect_price.left = rect_price.right - price_width;

    // 绘制名称
    if (data->info.is_ok && settings.showStockName)
    {
        pDC->SetTextColor(color_default);
        CString stock_name{data->GetDisplayName().c_str()};
        stock_name += _T(": ");
        CRect rect_name{rect};
        rect_name.right = rect_price.left;
        pDC->DrawText(stock_name, rect_name, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);
    }

    // 设置数值颜色
    if (settings.colorWithPrice)
    {
        if (data->realTimeData.displayFluctuation.find('-') != std::wstring::npos)
            pDC->SetTextColor(color_green);
        else
            pDC->SetTextColor(color_red);
    }
    else
    {
        pDC->SetTextColor(color_default);
    }

    // 绘制价格
    const wchar_t* priceText = data->realTimeData.displayPrice.empty()
        ? L"--" : data->realTimeData.displayPrice.c_str();
    pDC->DrawText(priceText, rect_price, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);

    // 绘制涨跌幅
    const wchar_t* fluctText = data->realTimeData.displayFluctuation.empty()
        ? L"--" : data->realTimeData.displayFluctuation.c_str();
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
    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;
    int space_width = pDC->GetTextExtent(_T(" ")).cx;
    int price_width = pDC->GetTextExtent(_T("99999.999")).cx;

    if (settings.displayMode == StockDisplayMode::ShowAll)
    {
        size_t numCols = (total + 1) / 2;

        std::vector<int> colWidths(numCols);
        for (size_t col = 0; col < numCols; col++)
        {
            size_t row1Idx = col * 2;
            size_t row2Idx = col * 2 + 1;

            int col1Width = (row1Idx < total) ? CalcStockDisplayWidth(pDC, codes[row1Idx], price_width, space_width, fluctuation_width) : 0;
            int col2Width = (row2Idx < total) ? CalcStockDisplayWidth(pDC, codes[row2Idx], price_width, space_width, fluctuation_width) : 0;
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
