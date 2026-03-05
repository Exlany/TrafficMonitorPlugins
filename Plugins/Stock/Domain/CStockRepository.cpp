#include "pch.h"
#include "CStockRepository.h"

namespace StockPlugin {
namespace Domain {

CStockRepository::CStockRepository() = default;

CStockRepository::~CStockRepository() = default;

std::shared_ptr<STOCK::StockData> CStockRepository::GetStockData(const std::wstring& code)
{
    return m_market.getStock(code);
}

void CStockRepository::ClearRealtimeData(bool includeOKX)
{
    m_market.ClearRealtimeData(includeOKX);
}

} // namespace Domain
} // namespace StockPlugin
