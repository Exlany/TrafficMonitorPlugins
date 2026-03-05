#pragma once
#include "StockItem.h"
#include "StockConstants.h"
#include <string>
#include "PluginInterface.h"
#include <map>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <atomic>

// 使用 StockConstants 命名空间中的常量
using namespace StockConstants;

class CManagerDialog;

class Stock : public ITMPlugin
{
private:
    Stock();
    virtual ~Stock();

public:
    static Stock &Instance();

    virtual IPluginItem *GetItem(int index) override;
    virtual const wchar_t *GetTooltipInfo() override;
    virtual void DataRequired() override;
    virtual OptionReturn ShowOptionsDialog(void *hParent) override;
    virtual const wchar_t *GetInfo(PluginInfoIndex index) override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data) override;
    virtual int GetCommandCount() override;
    virtual const wchar_t *GetCommandName(int command_index) override;
    virtual void OnPluginCommand(int command_index, void *hWnd, void *para) override;
    virtual void *GetPluginIcon() override;

    INT_PTR ShowStockManageDlg(CWnd *pWnd);
    void SendStockInfoRequest();
    void ShowContextMenu(CWnd *pWnd);
    void DisableUpdateCommand();
    void EnableUpdateCommand();

    void ShowFloatingWnd(void *hWnd, CPoint ptScreen, std::wstring stock_id);
    void DestroyFloatingWnd();
    void UpdateKLine();
    void SwitchToNextStock();  // 手动模式下切换到下一只股票
    size_t GetCurrentDisplayIndex() const { return m_current_display_index; }  // 获取当前显示的股票索引
    size_t GetSecondRowIndex();  // 获取第二行显示的股票索引

private:
    static UINT ThreadCallback(LPVOID dwUser);
    static UINT CheckUpdateThread(LPVOID pParam);  // 更新检查线程
    void LoadContextMenu();
    void updateItems();
    int GetSmartIndex();  // 获取涨跌幅最大的股票索引

private:
    static Stock m_instance;
    std::vector<StockItem> m_items;
    mutable std::shared_mutex m_itemsMutex;

    std::atomic<bool> m_is_thread_running{};
    CManagerDialog *m_option_dlg{};         // 保存选项设置对话框的句柄
    std::atomic<unsigned __int64> m_last_request_time{}; // 上次请求的时间
    CMenu m_menu;

    std::mutex m_wndMutex;
    std::unique_ptr<CFloatingWnd> m_pFloatingWnd;

    // 显示模式相关
    std::atomic<int> m_current_display_index{0};      // 当前显示的股票索引
    std::atomic<time_t> m_last_carousel_time{0};      // 上次轮播切换时间
};

#ifdef __cplusplus
extern "C"
{
#endif
    __declspec(dllexport) ITMPlugin *TMPluginGetInstance();

#ifdef __cplusplus
}
#endif
