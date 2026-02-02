#pragma once
#include <functional>
#include "common/ThreadUtil.hpp"
#include "common/TimeUtil.hpp"
#include "common/LockFreeQueue.hpp"
#include "common/Macros.hpp"
#include "common/Logging.hpp"
#include "exchange/order_server/ClientRequest.hpp"
#include "exchange/order_server/ClientResponse.hpp"
#include "exchange/market_data/MarketUpdate.hpp"
#include "MarketOrderBook.hpp"
#include "FeatureEngine.hpp"
#include "PositionKeeper.hpp"
#include "OrderManager.hpp"
#include "RiskManager.hpp"
#include "MarketMaker.hpp"
#include "LiquidityTaker.hpp"

namespace Trading { 
   class TradeEngine { 
    public : 
      TradeEngine(ClientID client_id_, 
                  AlgoType algo_type, 
                  const TradeEngineConfigHashMap  &ticker_config, 
                  Exchange::ClientRequestLFQueue  *client_request_, 
                  Exchange::ClientResponseLFQueue *client_response_, 
                  Exchange::MarketUpdateLFQueue   *market_updates); 

      ~TradeEngine(); 

      auto start() -> void;

      auto stop() -> void; 

      auto run() noexcept -> void; 

      auto sendClientRequest(const Exchange::MatchingEngineClientRequest *client_request) noexcept -> void; 
        
      auto onOrderBookUpdate(TickerID ticker_id, Price price, Side side, MarketOrderBook *book) noexcept -> void; 

      auto onTradeUpdate(const Exchange::MatchingEngineMarketUpdate *market_update, MarketOrderBook *book) noexcept -> void; 

      auto onOrderUpdate(const Exchange::MatchingEngineClientResponse *client_response) noexcept -> void; 
  
      auto initLastEventTime() { 
        last_event_time = Common::getCurrentNanos(); 
      }

      auto silentSeconds() { 
        return (Common::getCurrentNanos() - last_event_time) / NANOS_TO_SECS; 
      }

      auto getClientID() const { 
        return client_id; 
      }

     
      std::function<void(TickerID ticker_id, Price price, Side side, MarketOrderBook *book)>algoOnOrderBookUpdate; 
      std::function<void(const Exchange::MatchingEngineMarketUpdate *market_update, MarketOrderBook *book)>algoOnTradeUpdate; 
      std::function<void(const Exchange::MatchingEngineClientResponse *client_response)>algoOnOrderUpdate; 

      TradeEngine() = delete;

      TradeEngine(const TradeEngine &) = delete;

      TradeEngine(const TradeEngine &&) = delete;

      TradeEngine &operator=(const TradeEngine &) = delete;

      TradeEngine &operator=(const TradeEngine &&) = delete;


    private : 
      const ClientID client_id; 
      MarketOrderBookHashMap ticker_order_book; 
      Exchange::ClientRequestLFQueue  *outgoing_gateway_request  = nullptr; 
      Exchange::ClientResponseLFQueue *incoming_gateway_response = nullptr; 
      Exchange::MarketUpdateLFQueue    *incoming_md_updates      = nullptr; 

      Nanos last_event_time = 0; 
      volatile bool is_running = false; 
      std::string time_str; 
      Logger logger;

      FeatureEngine feature_engine; 
      PositionKeeper position_keeper; 
      OrderManager order_manager; 
      RiskManager  risk_manager; 
      MarketMaker    *maker_algo = nullptr; 
      LiquidityTaker *taker_algo = nullptr; 

      auto defaultAlgoOnOrderBookUpdate(TickerID ticker_id, Price price, Side side, MarketOrderBook *book) noexcept -> void { 
        logger.log("%:% %() % ticker:% price:% side:%\n",
            __FILE__, __LINE__, __FUNCTION__,
            Common::getCurrentTimeStr(&time_str),
            ticker_id, 
            Common::priceToString(price).c_str(),
            Common::sideToString(side).c_str()
        );
      }

      auto defaultAlgoOnTradeUpdate(const Exchange::MatchingEngineMarketUpdate *market_update, MarketOrderBook *) noexcept -> void { 
        logger.log("%:% %() % %\n", 
            __FILE__, __LINE__, __FUNCTION__, 
            Common::getCurrentTimeStr(&time_str),
            market_update->toString().c_str()
        );
      }

      auto defaultAlgoOnOrderUpdate(const Exchange::MatchingEngineClientResponse *client_response) noexcept -> void { 
        logger.log("%:% %() % %\n", 
            __FILE__, __LINE__, __FUNCTION__, 
            Common::getCurrentTimeStr(&time_str),
            client_response->toString().c_str()
        );
      }
   };  
}