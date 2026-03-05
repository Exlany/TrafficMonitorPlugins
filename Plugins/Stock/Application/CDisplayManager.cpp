#include "pch.h"
#include "CDisplayManager.h"
#include "../StockDef.h"
#include "../StockConstants.h"
#include <cmath>

namespace StockPlugin {
namespace Application {

CDisplayManager::CDisplayManager() = default;

CDisplayManager::~CDisplayManager() = default;

void CDisplayManager::SetSettings(Domain::CStockSettings* settings)
{
    m_settings = settings;
}

size_t CDisplayManager::Update()
{
    if (!m_settings)
        return 0;

    auto codes = m_settings->GetStockCodes();
    if (codes.empty())
        return 0;

    auto mode = m_settings->GetDisplayMode();

    switch (mode)
    {
    case Domain::StockDisplayMode::Carousel:
        // 轮播模式：检查是否需要切换
        if (codes.size() > 1)
        {
            time_t now = time(nullptr);
            if (now - m_lastCarouselTime >= m_settings->GetCarouselInterval())
            {
                m_lastCarouselTime = now;
                m_currentIndex = (m_currentIndex + 1) % codes.size();
            }
        }
        if (m_currentIndex >= codes.size())
            m_currentIndex = 0;
        break;

    case Domain::StockDisplayMode::Manual:
        // 手动模式：使用当前索引
        if (m_currentIndex >= codes.size())
            m_currentIndex = 0;
        break;

    case Domain::StockDisplayMode::Smart:
        // 智能模式由外部调用 GetSmartIndex 处理
        break;

    default:
        // ShowAll 模式不需要特殊处理
        break;
    }

    return m_currentIndex;
}

size_t CDisplayManager::GetCurrentIndex() const
{
    return m_currentIndex;
}

size_t CDisplayManager::GetSecondRowIndex() const
{
    if (!m_settings)
        return 0;

    auto codes = m_settings->GetStockCodes();
    if (codes.size() <= 1)
        return 0;

    return (m_currentIndex + 1) % codes.size();
}

void CDisplayManager::SwitchToNext()
{
    if (!m_settings)
        return;

    auto codes = m_settings->GetStockCodes();
    if (codes.size() > 1)
    {
        m_currentIndex = (m_currentIndex + 1) % codes.size();
    }
}

int CDisplayManager::GetSmartIndex(
    std::function<std::shared_ptr<STOCK::StockData>(const std::wstring&)> stockDataGetter) const
{
    if (!m_settings || !stockDataGetter)
        return 0;

    auto codes = m_settings->GetStockCodes();
    if (codes.empty())
        return 0;

    int maxIndex = 0;
    double maxScore = 0;

    for (size_t i = 0; i < codes.size(); i++)
    {
        auto data = stockDataGetter(codes[i]);
        if (data && data->realTimeData.prevClosePrice > 0)
        {
            // 计算当日涨跌幅绝对值
            double dailyChange = std::abs(data->realTimeData.currentPrice - data->realTimeData.prevClosePrice);
            double dailyChangePercent = dailyChange / data->realTimeData.prevClosePrice;

            // 计算近期涨跌幅（通过分时数据）
            double recentChangePercent = 0;
            auto timelineData = data->getTimelineData();
            if (timelineData && timelineData->data.size() > 1)
            {
                size_t dataSize = timelineData->data.size();
                size_t startIdx = (dataSize > StockConstants::RECENT_MINUTES) ?
                                  (dataSize - StockConstants::RECENT_MINUTES) : 0;

                STOCK::Price startPrice = timelineData->data[startIdx].price;
                STOCK::Price endPrice = timelineData->data[dataSize - 1].price;

                if (startPrice > 0)
                {
                    recentChangePercent = std::abs(endPrice - startPrice) / startPrice;
                }
            }

            // 综合评分
            double score = dailyChangePercent * StockConstants::DAILY_CHANGE_WEIGHT +
                           recentChangePercent * StockConstants::RECENT_CHANGE_WEIGHT;

            if (score > maxScore)
            {
                maxScore = score;
                maxIndex = static_cast<int>(i);
            }
        }
    }

    return maxIndex;
}

void CDisplayManager::Reset()
{
    m_currentIndex = 0;
    m_lastCarouselTime = 0;
}

} // namespace Application
} // namespace StockPlugin
