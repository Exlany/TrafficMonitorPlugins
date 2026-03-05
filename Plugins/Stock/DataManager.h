#pragma once
#include <string>
#include <map>
#include <memory>
#include <atomic>
#include "resource.h"
#include "StockDef.h"
#include <mutex>

// 新架构头文件
#include "Domain/CStockSettings.h"

// 前向声明新架构类
namespace StockPlugin {
namespace Infrastructure {
    class CIniConfigStore;
    class CWinHttpClient;
    class CResourceCache;
}
namespace Domain {
    class CStockRepository;
    class CStockCodeResolver;
}
}

#define g_data CDataManager::Instance()

// 使用新架构的类型别名
using StockDisplayMode = StockPlugin::Domain::StockDisplayMode;
using SettingsSnapshot = StockPlugin::Domain::SettingsSnapshot;

/// @brief 数据管理器
/// @details 统一管理配置、资源和数据访问
class CDataManager
{
private:
    CDataManager();
    ~CDataManager();

public:
    static CDataManager &Instance();

    void LoadConfig(const std::wstring &config_dir);
    void SaveConfig();
    const CString &StringRes(UINT id); // 根据资源id获取一个字符串资源
    void DPIFromWindow(CWnd *pWnd);
    int DPI(int pixel);
    float DPIF(float pixel);
    int RDPI(int pixel);
    HICON GetIcon(UINT id);
    void ResetText();
    std::shared_ptr<STOCK::StockData> GetStockData(const std::wstring &code);

    // 获取最新数据
    void RequestRealtimeData();
    void RequestTimelineData(std::wstring stock_id);
    void RequestOKXData(const std::wstring& code);  // OKX虚拟货币数据

    // 新架构访问器
    StockPlugin::Domain::CStockSettings& GetSettings();
    StockPlugin::Domain::CStockRepository& GetRepository();
    StockPlugin::Domain::CStockCodeResolver& GetCodeResolver();
    StockPlugin::Infrastructure::CWinHttpClient& GetHttpClient();
    StockPlugin::Infrastructure::CResourceCache& GetResourceCache();

    // 线程安全的设置数据访问器
    /// @brief 获取设置数据的快照（线程安全）
    SettingsSnapshot GetSettingsSnapshot() const;
    /// @brief 更新设置数据（线程安全）
    void UpdateSettings(const SettingsSnapshot& snapshot);

    // 公共成员
    std::wstring m_log_path;
    std::atomic<bool> m_right_align{false}; // 数值是否右对齐（原子类型保证线程安全）
    UINT m_system_code_page{}; // 系统默认代码页（936=GBK, 65001=UTF-8）

    std::mutex& GetStockDataMutex();

private:
    static CDataManager m_instance;

    std::wstring m_config_path;

    // 旧架构成员（逐步迁移）
    std::map<UINT, CString> m_string_table;
    mutable std::mutex m_stringTableMutex;  // 保护 m_string_table
    std::map<UINT, HICON> m_icons;
    mutable std::mutex m_iconsMutex;  // 保护 m_icons
    std::atomic<int> m_dpi{96};  // 原子类型保证线程安全

    // 新架构成员
    std::unique_ptr<StockPlugin::Infrastructure::CIniConfigStore> m_configStore;
    std::unique_ptr<StockPlugin::Infrastructure::CWinHttpClient> m_httpClient;
    std::unique_ptr<StockPlugin::Infrastructure::CResourceCache> m_resourceCache;
    std::unique_ptr<StockPlugin::Domain::CStockSettings> m_settings;
    std::unique_ptr<StockPlugin::Domain::CStockRepository> m_repository;
    std::unique_ptr<StockPlugin::Domain::CStockCodeResolver> m_codeResolver;

    void InitNewArchitecture();
};
