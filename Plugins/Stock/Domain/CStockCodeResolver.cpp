#include "pch.h"
#include "CStockCodeResolver.h"
#include "../StockMappings.h"
#include <algorithm>
#include <vector>

namespace StockPlugin {
namespace Domain {

CStockCodeResolver::CStockCodeResolver() = default;

CStockCodeResolver::~CStockCodeResolver() = default;

bool CStockCodeResolver::HasPrefix(const std::wstring& code) const
{
    return code.find(L"sz") == 0 || code.find(L"sh") == 0 || code.find(L"bj") == 0 ||
           code.find(L"rt_hk") == 0 || code.find(L"gb_") == 0 || code.find(L"int_") == 0 ||
           code.find(L"okx_") == 0 || code.find(L"hf_") == 0 || code.find(L"bn_") == 0 ||
           code.find(L"fx_") == 0;
}

std::wstring CStockCodeResolver::Resolve(const std::wstring& input) const
{
    // 委托给 StockMappings 命名空间
    return StockMappings::SmartStockCode(input);
}

StockConstants::ProductType CStockCodeResolver::GetProductType(const std::wstring& code) const
{
    // 委托给 StockMappings 命名空间
    return StockMappings::GetProductType(code);
}

int CStockCodeResolver::GetTypeIndex(const std::wstring& code) const
{
    // 委托给 StockMappings 命名空间
    return StockMappings::GetStockTypeIndex(code);
}

std::wstring CStockCodeResolver::SmartShortName(const std::wstring& name) const
{
    // 委托给 StockMappings 命名空间
    return StockMappings::SmartShortName(name);
}

} // namespace Domain
} // namespace StockPlugin
