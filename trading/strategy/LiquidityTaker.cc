#include "LiquidityTaker.hpp"
#include "TradeEngine.hpp" 

namespace Trading {  
  LiquidityTaker::LiquidityTaker(Common::Logger *logger_, 
                TradeEngine *trade_engine, 
                const FeatureEngine *feature_engine_, 
                OrderManager *order_manager_, 
                const TradeEngineConfigHashMap &ticker_config_) : 
    feature_engine(feature_engine_), 
    order_manager(order_manager_), 
    logger(logger_), 
    ticker_config(ticker_config_) { 
    trade_engine->algoOnOrderBookUpdate = [this](TickerID ticker_id, Price price, Side side, auto book) { 
        onOrderBookUpdate(ticker_id, price, side, book); 
    }; 
    trade_engine->algoOnTradeUpdate = [this](auto market_update, auto book) { 
        onTradeUpdate(market_update, book); 
    }; 
    trade_engine->algoOnOrderUpdate = [this](auto client_response) { 
        onOrderUpdate(client_response); 
    }; 
  }
  auto LiquidityTaker::onTradeUpdate(const Exchange::MatchingEngineMarketUpdate *market_update, MarketOrderBook *book) noexcept -> void { 
    logger->log("%:% %() % %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str),
        market_update->toString().c_str()
    );
    const auto bbo = book->getBBO(); 
    const auto agg_qty_ratio = feature_engine->getAggTradeQtyRatio(); 
    if(LIKELY(bbo->best_bid_price != PRICE_INVALID && bbo->best_ask_price != PRICE_INVALID && 
               agg_qty_ratio != FEATURE_INVALID)) { 
        logger->log("%:% %() % % agg-qty-ratio:%\n",
          __FILE__, __LINE__, __FUNCTION__,
          Common::getCurrentTimeStr(&time_str),
          bbo->toString().c_str(),
          agg_qty_ratio
        );
        const auto clip      = ticker_config.at(market_update->ticker_id).clip; 
        const auto threshold = ticker_config.at(market_update->ticker_id).threshold; 
        if(agg_qty_ratio >= threshold) { 
          if(market_update->side == Side::BUY) { 
            order_manager->moveOrders(market_update->ticker_id, bbo->best_ask_price, PRICE_INVALID, clip); 
          } else { 
            order_manager->moveOrders(market_update->ticker_id, PRICE_INVALID, bbo->best_bid_price, clip); 
          }
        }
    }
  }

  auto LiquidityTaker::onOrderBookUpdate(TickerID ticker_id, Price price, Side side, const MarketOrderBook *book) noexcept -> void { 
    logger->log("%:% %() % ticker:% price:% side:%\n", 
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str), 
        ticker_id, 
        Common::priceToString(price).c_str(),
        Common::sideToString(side).c_str()
    );
  }

  auto LiquidityTaker::onOrderUpdate(const Exchange::MatchingEngineClientResponse *client_response) noexcept -> void { 
    logger->log("%:% %() % %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str),
        client_response->toString().c_str()
    );
    order_manager->onOrderUpdate(client_response);
  }
} 