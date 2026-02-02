#pragma once 

#include "common/Macros.hpp"
#include "common/Logging.hpp"

#include "OrderManager.hpp"
#include "FeatureEngine.hpp"

using namespace Common; 

namespace Trading { 
 class LiquidityTaker { 
  public: 
    LiquidityTaker(Common::Logger *logger_, 
                TradeEngine *trade_engine, 
                const FeatureEngine *feature_engine_, 
                OrderManager *order_manager_, 
                const TradeEngineConfigHashMap &ticker_config_); 

    auto onOrderBookUpdate(TickerID ticker_id, Price price, Side side, const MarketOrderBook *book) noexcept -> void; 

    auto onTradeUpdate(const Exchange::MatchingEngineMarketUpdate *market_update, MarketOrderBook *book) noexcept -> void; 

    auto onOrderUpdate(const Exchange::MatchingEngineClientResponse *client_response) noexcept -> void; 
  
    LiquidityTaker() = delete;

    LiquidityTaker(const LiquidityTaker &) = delete;

    LiquidityTaker(const LiquidityTaker &&) = delete;

    LiquidityTaker &operator=(const LiquidityTaker &) = delete;

    LiquidityTaker &operator=(const LiquidityTaker &&) = delete;
  
  private: 
    const FeatureEngine *feature_engine = nullptr; 
    OrderManager *order_manager = nullptr; 
    std::string time_str;
    Common::Logger *logger = nullptr; 
    const TradeEngineConfigHashMap ticker_config;
 }; 
}