#include "MarketMaker.hpp"

#include "TradeEngine.hpp"

namespace Trading { 
  MarketMaker::MarketMaker(Common::Logger *logger_, 
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
  auto MarketMaker::onOrderBookUpdate(TickerID ticker_id, Price price, Side side, const MarketOrderBook *book) noexcept -> void { 
    logger->log("%:% %() % ticker:% price:% side:%\n",
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str),
        ticker_id, 
        Common::priceToString(price).c_str(),
        Common::sideToString(side).c_str()
    );

    const auto bbo = book->getBBO(); 
    const auto fair_price = feature_engine->getMarketPrice(); 
    if(LIKELY(bbo->best_bid_price != PRICE_INVALID && bbo->best_ask_price != PRICE_INVALID && 
              fair_price != FEATURE_INVALID)) { 
      logger->log("%:% %() % % fair-price:%\n", 
                  __FILE__, __LINE__, __FUNCTION__,
                  Common::getCurrentTimeStr(&time_str),
                  bbo->toString().c_str(), 
                  fair_price
      );
      const auto clip = ticker_config.at(ticker_id).clip; 
      const auto threshold = ticker_config.at(ticker_id).threshold; 
      const auto bid_price = bbo->best_bid_price - (fair_price - bbo->best_bid_price >= threshold ? 0 : 1);
      const auto ask_price = bbo->best_ask_price + (bbo->best_ask_price - fair_price >= threshold ? 0 : 1); 
      order_manager->moveOrders(ticker_id, bid_price, ask_price, clip); 
    }
  }
  
  auto MarketMaker::onTradeUpdate(const Exchange::MatchingEngineMarketUpdate *market_update, MarketOrderBook *book) noexcept -> void { 
    logger->log("%:% %() % %\n", 
      __FILE__, __LINE__, __FUNCTION__, 
      Common::getCurrentTimeStr(&time_str),
      market_update->toString().c_str()
    );
  }

  auto MarketMaker::onOrderUpdate(const Exchange::MatchingEngineClientResponse *client_response) noexcept -> void { 
    logger->log("%:% %() % %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str),
        client_response->toString().c_str()
    );
    order_manager->onOrderUpdate(client_response);
  }
}