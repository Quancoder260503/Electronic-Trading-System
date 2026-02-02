#pragma once 
#include "common/Macros.hpp"
#include "common/Types.hpp"
#include "common/Logging.hpp"
#include "MarketOrderBook.hpp"

using namespace Common; 

namespace Trading { 
 constexpr auto FEATURE_INVALID = std::numeric_limits<double>::quiet_NaN(); 
 class FeatureEngine { 
  public:
    FeatureEngine(Common::Logger *logger_) : logger(logger_) { }

    auto onOrderBookUpdate(TickerID ticker_id, Price price, Side side, MarketOrderBook *book) noexcept -> void { 
      const auto bbo = book->getBBO(); 
      if(LIKELY(bbo->best_bid_price != PRICE_INVALID && bbo->best_ask_price != PRICE_INVALID)) { 
        market_price = (bbo->best_bid_price * bbo->best_ask_quantity + 
                        bbo->best_ask_price * bbo->best_bid_quantity) / 
                        static_cast<double>(bbo->best_ask_quantity + bbo->best_bid_quantity); 
      }
      logger->log("%:% %() % ticker:% price:% side:% mkt-price:% agg-trade-ratio:%\n",
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str),
        ticker_id, 
        Common::priceToString(price).c_str(),
        Common::sideToString(side).c_str(),
        market_price, 
        agg_trade_qty_ratio
      );
    }

    auto onTradeUpdate(const Exchange::MatchingEngineMarketUpdate *market_update, MarketOrderBook *book) noexcept -> void { 
      const auto bbo = book->getBBO(); 
      if(LIKELY(bbo->best_bid_price != PRICE_INVALID && bbo->best_ask_price != PRICE_INVALID)) { 
        agg_trade_qty_ratio = static_cast<double>(market_update->quantity) / 
                              static_cast<double>(market_update->side == Side::BUY ? bbo->best_bid_quantity : bbo->best_ask_quantity); 
      }
      logger->log("%:% %() % % mkt-price:% agg-trade-ratio:%\n",
         __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str),
        market_update->toString().c_str(),
        market_price, 
        agg_trade_qty_ratio
      );
    }

    auto getMarketPrice() const noexcept { 
      return market_price; 
    }

    auto getAggTradeQtyRatio() const noexcept { 
      return agg_trade_qty_ratio; 
    }


    FeatureEngine() = delete;

    FeatureEngine(const FeatureEngine &) = delete;

    FeatureEngine(const FeatureEngine &&) = delete;

    FeatureEngine &operator=(const FeatureEngine &) = delete;

    FeatureEngine &operator=(const FeatureEngine &&) = delete;

  private: 
    std::string time_str;
    Common::Logger *logger = nullptr; 
    double market_price        = FEATURE_INVALID; 
    double agg_trade_qty_ratio = FEATURE_INVALID; 
 }; 
}