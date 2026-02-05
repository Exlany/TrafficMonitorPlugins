#include "pch.h"
#include "StockItem.h"
#include "DataManager.h"
#include "Stock.h"
#include "Common.h"
#include <algorithm>
#include "FloatingWnd.h"
#undef min
#undef max

const wchar_t *StockItem::GetItemName() const
{
    std::lock_guard<std::mutex> lock(Stock::Instance().m_stockDataMutex);
    auto data = g_data.GetStockData(stock_id);

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

    std::lock_guard<std::mutex> lock(Stock::Instance().m_stockDataMutex);

    auto& codes = g_data.m_setting_data.m_stock_codes;

    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;
    int space_width = pDC->GetTextExtent(_T(" ")).cx;
    int itemSpacing = pDC->GetTextExtent(_T("  ")).cx;

    // 计算单个股票的实际宽度（确保返回合理的最小宽度）
    auto calcSingleWidth = [&](const std::wstring& code) -> int {
        auto data = g_data.GetStockData(code);

        // 根据实际价格计算宽度
        int price_width;
        if (data != nullptr && !data->realTimeData.displayPrice.empty())
        {
            price_width = pDC->GetTextExtent(data->realTimeData.displayPrice.c_str()).cx;
        }
        else
        {
            // 数据未加载时使用默认宽度
            price_width = pDC->GetTextExtent(_T("--")).cx;
        }

        int width = price_width + space_width + fluctuation_width;

        if (g_data.m_setting_data.m_show_stock_name)
        {
            if (data != nullptr && !data->GetDisplayName().empty())
            {
                CString stock_name{data->GetDisplayName().c_str()};
                stock_name += _T(": ");
                width += pDC->GetTextExtent(stock_name).cx;
            }
            else
            {
                // 数据未加载时使用股票代码作为名称宽度估算
                CString stock_name{code.c_str()};
                stock_name += _T(": ");
                width += pDC->GetTextExtent(stock_name).cx;
            }
        }
        return width;
    };

    if (codes.size() >= 2)
    {
        if (g_data.m_setting_data.m_display_mode == StockDisplayMode::ShowAll)
        {
            // ShowAll模式：计算每行实际宽度
            size_t total = codes.size();

            // 第一行宽度（索引0,2,4...）
            int row1Width = 0;
            for (size_t i = 0; i < total; i += 2)
            {
                if (row1Width > 0) row1Width += itemSpacing;
                row1Width += calcSingleWidth(codes[i]);
            }

            // 第二行宽度（索引1,3,5...）
            int row2Width = 0;
            for (size_t i = 1; i < total; i += 2)
            {
                if (row2Width > 0) row2Width += itemSpacing;
                row2Width += calcSingleWidth(codes[i]);
            }

            return max(row1Width, row2Width);
        }
        else
        {
            // 非ShowAll模式：只显示2个股票，每行1个，都按模式变化
            size_t firstIdx = Stock::Instance().GetCurrentDisplayIndex();
            size_t secondIdx = Stock::Instance().GetSecondRowIndex();

            int row1Width = (firstIdx < codes.size()) ? calcSingleWidth(codes[firstIdx]) : 0;
            int row2Width = (secondIdx < codes.size()) ? calcSingleWidth(codes[secondIdx]) : 0;

            return max(row1Width, row2Width);
        }
    }

    // 单个股票
    if (!codes.empty())
    {
        return calcSingleWidth(codes[0]);
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

    std::lock_guard<std::mutex> lock(Stock::Instance().m_stockDataMutex);

    // 文本颜色
    COLORREF color_default = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    COLORREF color_red = dark_mode ? RGB(255, 121, 120) : RGB(195, 0, 0);
    COLORREF color_green = dark_mode ? RGB(111, 215, 149) : RGB(46, 139, 87);

    auto& codes = g_data.m_setting_data.m_stock_codes;

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

    DrawSingleStock(pDC, currentStockId, x, y, w, h, color_default, color_red, color_green);
}

const wchar_t *StockItem::GetItemValueSampleText() const
{
    //    if (g_data.m_setting_data.m_show_stock_name)
    //    {
    //        return L"--------: 0000000.00 +00.00%";
    //    }
    //    else
    //    {
    //        return L"0000000.00 +00.00%";
    //    }
    return L"";
}

int StockItem::OnMouseEvent(MouseEventType type, int x, int y, void *hWnd, int flag)
{
    CWnd *pWnd = CWnd::FromHandle((HWND)hWnd);
    LogX(L"OnMouseEvent: %d\n", type);

    auto& codes = g_data.m_setting_data.m_stock_codes;

    switch (type)
    {
    case IPluginItem::MT_RCLICKED:
        Stock::Instance().ShowContextMenu(pWnd);
        return 1;

    case IPluginItem::MT_LCLICKED:
    {
        // 非ShowAll模式下，单击不触发走势图（避免与双击切换冲突）
        if (g_data.m_setting_data.m_display_mode != StockDisplayMode::ShowAll)
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
        if (g_data.m_setting_data.m_display_mode != StockDisplayMode::ShowAll)
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

void StockItem::DrawSingleStock(CDC *pDC, const std::wstring& code, int x, int y, int w, int h,
    COLORREF color_default, COLORREF color_red, COLORREF color_green)
{
    auto data = g_data.GetStockData(code);
    if (data == nullptr)
        return;

    CRect rect(CPoint(x, y), CSize(w, h));

    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;
    int price_width = pDC->GetTextExtent(_T("999.999")).cx;

    CRect rect_fluctuation{rect};
    rect_fluctuation.left = rect.right - fluctuation_width;

    CRect rect_price{rect};
    rect_price.right = rect_fluctuation.left - pDC->GetTextExtent(_T(" ")).cx;
    rect_price.left = rect_price.right - price_width;

    // 绘制名称
    if (data->info.is_ok && g_data.m_setting_data.m_show_stock_name)
    {
        pDC->SetTextColor(color_default);
        CString stock_name{data->GetDisplayName().c_str()};
        stock_name += _T(": ");
        CRect rect_name{rect};
        rect_name.right = rect_price.left;
        pDC->DrawText(stock_name, rect_name, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);
    }

    // 设置数值颜色
    if (g_data.m_setting_data.m_color_with_price)
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
    auto& codes = g_data.m_setting_data.m_stock_codes;
    size_t total = codes.size();
    if (total < 2)
        return;

    int rowHeight = h / 2;
    int itemSpacing = pDC->GetTextExtent(_T("  ")).cx;
    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;
    int space_width = pDC->GetTextExtent(_T(" ")).cx;

    // 计算单个股票的实际宽度（与GetItemWidthEx保持一致）
    auto calcStockWidth = [&](const std::wstring& code) -> int {
        auto data = g_data.GetStockData(code);

        int price_width;
        if (data != nullptr && !data->realTimeData.displayPrice.empty())
        {
            price_width = pDC->GetTextExtent(data->realTimeData.displayPrice.c_str()).cx;
        }
        else
        {
            price_width = pDC->GetTextExtent(_T("--")).cx;
        }

        int width = price_width + space_width + fluctuation_width;

        if (g_data.m_setting_data.m_show_stock_name)
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
    };

    if (g_data.m_setting_data.m_display_mode == StockDisplayMode::ShowAll)
    {
        // ShowAll模式：显示所有股票，2xN矩阵布局，使用实际宽度
        // 第一行包含偶数索引：0,2,4...
        // 第二行包含奇数索引：1,3,5...

        // 计算第一行的最后一个偶数索引
        int lastEvenIdx = static_cast<int>((total - 1) / 2) * 2;  // 0,2,4... 中最大的
        // 计算第二行的最后一个奇数索引
        int lastOddIdx = (total >= 2) ? (static_cast<int>((total - 2) / 2) * 2 + 1) : -1;  // 1,3,5... 中最大的

        // 绘制第一行（索引0,2,4...从右向左）
        int currentX = x + w;
        for (int i = lastEvenIdx; i >= 0; i -= 2)
        {
            int stockWidth = calcStockWidth(codes[i]);
            currentX -= stockWidth;
            DrawSingleStockInRow(pDC, codes[i], currentX, y, stockWidth, rowHeight,
                color_default, color_red, color_green, 0);
            currentX -= itemSpacing;
        }

        // 绘制第二行（索引1,3,5...从右向左）
        currentX = x + w;
        for (int i = lastOddIdx; i >= 1; i -= 2)
        {
            int stockWidth = calcStockWidth(codes[i]);
            currentX -= stockWidth;
            DrawSingleStockInRow(pDC, codes[i], currentX, y + rowHeight, stockWidth, rowHeight,
                color_default, color_red, color_green, 0);
            currentX -= itemSpacing;
        }
    }
    else
    {
        // 非ShowAll模式：只显示2个股票，两行都按模式变化
        size_t firstIdx = Stock::Instance().GetCurrentDisplayIndex();
        size_t secondIdx = Stock::Instance().GetSecondRowIndex();

        std::wstring firstCode = (firstIdx < total) ? codes[firstIdx] : codes[0];
        std::wstring secondCode = (secondIdx < total) ? codes[secondIdx] : codes[0];

        // 绘制第一行
        DrawSingleStockInRow(pDC, firstCode, x, y, w, rowHeight,
            color_default, color_red, color_green, 0);

        // 绘制第二行
        DrawSingleStockInRow(pDC, secondCode, x, y + rowHeight, w, rowHeight,
            color_default, color_red, color_green, 0);
    }
}

void StockItem::DrawSingleStockInRow(CDC *pDC, const std::wstring& code, int x, int y, int w, int h,
    COLORREF color_default, COLORREF color_red, COLORREF color_green, int fixedNameWidth)
{
    auto data = g_data.GetStockData(code);
    if (data == nullptr)
        return;

    CRect rect(CPoint(x, y), CSize(w, h));

    int fluctuation_width = pDC->GetTextExtent(_T("-99.99%")).cx;

    // 使用实际价格宽度
    int price_width;
    if (!data->realTimeData.displayPrice.empty())
    {
        price_width = pDC->GetTextExtent(data->realTimeData.displayPrice.c_str()).cx;
    }
    else
    {
        price_width = pDC->GetTextExtent(_T("--")).cx;
    }

    CRect rect_fluctuation{rect};
    rect_fluctuation.left = rect.right - fluctuation_width;

    CRect rect_price{rect};
    rect_price.right = rect_fluctuation.left - pDC->GetTextExtent(_T(" ")).cx;
    rect_price.left = rect_price.right - price_width;

    // 绘制名称
    if (data->info.is_ok && g_data.m_setting_data.m_show_stock_name)
    {
        pDC->SetTextColor(color_default);
        CString stock_name{data->GetDisplayName().c_str()};
        stock_name += _T(": ");
        CRect rect_name{rect};
        rect_name.right = rect_price.left;
        pDC->DrawText(stock_name, rect_name, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);
    }

    // 设置数值颜色
    if (g_data.m_setting_data.m_color_with_price)
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
