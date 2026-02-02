#include "TradeEngine.hpp"

namespace Trading { 
  TradeEngine::TradeEngine(ClientID client_id_, 
                  AlgoType algo_type, 
                  const TradeEngineConfigHashMap  &ticker_config, 
                  Exchange::ClientRequestLFQueue  *client_request_, 
                  Exchange::ClientResponseLFQueue *client_response_, 
                  Exchange::MarketUpdateLFQueue   *market_updates) : 
  client_id(client_id_), 
  outgoing_gateway_request(client_request_), 
  incoming_gateway_response(client_response_), 
  incoming_md_updates(market_updates),
  logger("trading_engine_" + std::to_string(client_id_) + ".log"), 
  feature_engine(&logger), 
  position_keeper(&logger),  
  risk_manager(&logger, &position_keeper, ticker_config), 
  order_manager(&logger,  this, risk_manager)  { 

    for(size_t i = 0; i < ticker_order_book.size(); i++) { 
        ticker_order_book[i] = new MarketOrderBook(i, &logger); 
        ticker_order_book[i]->setTradeEngine(this); 
    }
    algoOnOrderBookUpdate = [this](auto ticker_id, auto price, auto side, auto book)  { 
      defaultAlgoOnOrderBookUpdate(ticker_id, price, side, book);  
    }; 
    algoOnTradeUpdate = [this](auto market_update, auto book) { 
      defaultAlgoOnTradeUpdate(market_update, book); 
    }; 
    algoOnOrderUpdate = [this](auto client_response) { 
       defaultAlgoOnOrderUpdate(client_response); 
    }; 

    if(algo_type == AlgoType::MAKER) { 
      maker_algo = new MarketMaker(&logger, this, &feature_engine, &order_manager, ticker_config); 
    } else { 
      taker_algo = new LiquidityTaker(&logger, this, &feature_engine, &order_manager, ticker_config);   
    }
    for(TickerID i = 0; i < ticker_config.size(); i++) { 
        logger.log("%:% %() % Initialized % Ticker:% %.\n", 
            __FILE__, __LINE__, __FUNCTION__,
            Common::getCurrentTimeStr(&time_str),
            algoTypeToString(algo_type), 
            i,
            ticker_config.at(i).toString()
        );
    }
  }

  TradeEngine::~TradeEngine() { 
    is_running = false;
    
    using namespace std::literals::chrono_literals; 
    std::this_thread::sleep_for(1s); 
    delete maker_algo; 
    maker_algo = nullptr; 
    delete taker_algo; 
    taker_algo = nullptr; 

    for(auto &order_book : ticker_order_book) { 
      delete order_book; 
      order_book = nullptr; 
    }
    outgoing_gateway_request  = nullptr; 
    incoming_gateway_response = nullptr; 
    incoming_md_updates       = nullptr; 
  }
  
  auto TradeEngine::start() noexcept -> void { 
    is_running = true; 
    ASSERT(Common::createAndStartThread(-1, "Trading/TradeEngine", [this]() { 
      run();}) != nullptr, "Failed to start Trade Engine Thread."); 
  }

  auto TradeEngine::run() noexcept -> void { 
    logger.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
    while(is_running) { 
      for(auto client_response = incoming_gateway_response->getNextToRead(); 
         incoming_gateway_response->size() && client_response; 
         client_response = incoming_gateway_response->getNextToRead()) { 
         logger.log("%:% %() % Processing %\n", 
            __FILE__, __LINE__, __FUNCTION__, 
            Common::getCurrentTimeStr(&time_str),
            client_response->toString().c_str()
         );
         onOrderUpdate(client_response); 
         incoming_gateway_response->updateReadIndex(); 
         last_event_time = Common::getCurrentNanos(); 
      }

      for(auto market_update = incoming_md_updates->getNextToRead(); 
          incoming_md_updates->size() && market_update; 
          market_update = incoming_md_updates->getNextToRead()) { 
        ASSERT(market_update->ticker_id < ticker_order_book.size(), 
         "Unknown ticker-id on update:" + market_update->toString()
        );
        ticker_order_book[market_update->ticker_id]->onMarketUpdate(market_update); 
        incoming_md_updates->updateReadIndex(); 
        last_event_time = Common::getCurrentNanos(); 
      }
    }
  }

  auto TradeEngine::stop() noexcept -> void { 
   while(incoming_gateway_response->size() || incoming_md_updates->size()) { 
      logger.log("%:% %() % Sleeping till all updates are consumed ogw-size:% md-size:%\n", 
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str), 
        incoming_gateway_response->size(), 
        incoming_md_updates->size()
      );
      using namespace std::literals::chrono_literals;
      std::this_thread::sleep_for(10ms);
   }
   logger.log("%:% %() % POSITIONS\n%\n", 
    __FILE__, __LINE__, __FUNCTION__, 
    Common::getCurrentTimeStr(&time_str),
    position_keeper.toString()
   );
    is_running = false; 
  }

  auto TradeEngine::sendClientRequest(const Exchange::MatchingEngineClientRequest *client_request) noexcept -> void { 
    logger.log("%:% %() % Sending %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str),
        client_request->toString().c_str()
    );
    auto next_write = outgoing_gateway_request->getNextToWrite(); 
    *next_write = std::move(*client_request); 
    outgoing_gateway_request->updateWriteIndex(); 
  }


   auto TradeEngine::onOrderBookUpdate(TickerID ticker_id, Price price, Side side,  MarketOrderBook *book) noexcept -> void { 
     logger.log("%:% %() % ticker:% price:% side:%\n", 
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str), 
        ticker_id, 
        Common::priceToString(price).c_str(),
        Common::sideToString(side).c_str()
     );
     position_keeper.updateBBO(ticker_id, book->getBBO()); 
     feature_engine.onOrderBookUpdate(ticker_id, price, side, book); 

     algoOnOrderBookUpdate(ticker_id, price, side, book); 
   } 

   auto TradeEngine::onTradeUpdate(const Exchange::MatchingEngineMarketUpdate *market_update, MarketOrderBook *book) noexcept -> void { 
    logger.log("%:% %() % %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str),
        market_update->toString().c_str()
    );   
    feature_engine.onTradeUpdate(market_update, book); 
    algoOnTradeUpdate(market_update, book); 
   }

   auto TradeEngine::onOrderUpdate(const Exchange::MatchingEngineClientResponse *client_response) noexcept -> void {
     logger.log("%:% %() % %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str),
         client_response->toString().c_str()
     );
     if(UNLIKELY(client_response->type == Exchange::ClientResponseType::FILLED)) { 
      position_keeper.addFill(client_response); 
     }
     algoOnOrderUpdate(client_response); 
  }
}