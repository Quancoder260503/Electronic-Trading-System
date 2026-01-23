#include "SnapshotSynthesizer.hpp"

namespace Exchange { 
 SnapshotSynthesizer::SnapshotSynthesizer(
    MDPMarketUpdaLFQueue *market_updates, 
    const std::string &iface, 
    const std::string &snapshot_ip, 
    int snapshot_port) : 
    snapshot_md_updates(market_updates), 
    logger("exchange_snapshot_synthesizer.log"), 
    snapshot_socket(logger), 
    order_pool(MATCHING_ENGINE_MAX_ORDER_IDS)
    { 
        ASSERT(snapshot_socket.init(snapshot_ip, iface, snapshot_port, false) >= 0, 
         "Unable to create snapshot mcast socket. error: " + std::string(std::error(errno)));
        for(auto &orders : ticker_orders) { 
            orders.fill(nullptr); 
        }  
    }
 SnapshotSynthesizer::~SnapshotSynthesizer() { 
   stop(); 
 }   

 void SnapshotSynthesizer::start() { 
  is_running = true; 
  ASSERT(Common::createAndStartThread(-1, "Exchange/SnapshotSynthesizer", [this]() { 
    run(); }) != nullptr, "Failed to start SnapshotSynthesizer thread."); 
 }

 void SnapshotSynthesizer::stop() { 
  is_running = false; 
 }

 auto SnapshotSynthesizer::addToSnapshot(const MDPMarketUpdate *market_update) { 
   const auto &me_market_update = market_update->me_market_update;
   auto *orders = &ticker_orders.at(me_market_update.ticker_id);
   switch (me_market_update.type) { 
     case MarketUpdateType::ADD : { 
       auto order = orders->at(me_market_update.order_id); 
       ASSERT(order == nullptr, "Received : " + me_market_update.toString() + " but already exists : " + (order ? order->toString() : "")); 
       orders->at(me_market_update.order_id) = order_pool.allocate(me_market_update); 
     }
     break;
     
     case MarketUpdateType::MODIFY : { 
      auto order = orders->at(me_market_update.order_id); 
      ASSERT(order != nullptr, "Received : " + me_market_update.toString() + " but order does not exist.");
      ASSERT(order->order_id == me_market_update.order_id, "Expecting existing order to match the new one."); 
      ASSERT(order->side     == me_market_update.side    , "Expecting existing order to match the new one."); 
      order->quantity = me_market_update.quantity; 
      order->price    = me_market_update.price; 
     }
     break; 

     case MarketUpdateType::CANCEL : { 
      auto order = orders->at(me_market_update.order_id); 
      ASSERT(order != nullptr, "Received : " + me_market_update.toString() + " but order does not exist.");
      ASSERT(order->order_id == me_market_update.order_id, "Expecting existing order to match the new one."); 
      ASSERT(order->side     == me_market_update.side    , "Expecting existing order to match the new one."); 
      order_pool.deallocate(order); 
      orders->at(me_market_update.order_id) = nullptr;  
    }
    break; 
    case MarketUpdateType::SNAPSHOT_START:
    case MarketUpdateType::CLEAR: 
    case MarketUpdateType::SNAPSHOT_END: 
    case MarketUpdateType::TRADE:
    case MarketUpdateType::INVALID:
     break; 
   }
   ASSERT(market_update->sequence_number == last_increment_sequence_number + 1, 
    "Expected incremental sequence number to increase"); 
   last_increment_sequence_number = market_update->sequence_number; 
 }


 auto SnapshotSynthesizer::publishSnapshot() { 
  size_t snapshot_size = 0; 
  const MDPMarketUpdate start_market_update { 
   snapshot_size++, 
   {
    MarketUpdateType::SNAPSHOT_START, 
    last_increment_sequence_number
   }
  }; 

  logger.log("%:% %() % %\n", 
    __FILE__, __LINE__, __FUNCTION__, 
    getCurrentTimeStr(&time_str), start_market_update.toString()
  ); 

  snapshot_socket.send(&start_market_update, sizeof(MDPMarketUpdate)); 
  snapshot_socket.sendAndRecv(); 
  for(size_t ticker_id = 0; ticker_id < ticker_orders.size(); ticker_id++) { 
    const auto &orders = ticker_orders.at(ticker_id); 

    MatchingEngineMarketUpdate me_market_update; 
    me_market_update.type = MarketUpdateType::CLEAR; 
    me_market_update.ticker_id = ticker_id; 

    const MDPMarketUpdate clear_market_update{ 
      snapshot_size++, 
      me_market_update
    }; 
    
    logger.log("%:% %() % %\n", 
      __FILE__, __LINE__, __FUNCTION__, 
     getCurrentTimeStr(&time_str), clear_market_update.toString()
    ); 
    snapshot_socket.send(&clear_market_update, sizeof(MDPMarketUpdate)); 

    for(const auto order : orders) { 
     if(order) { 
      const MDPMarketUpdate propagate_market_update { 
       snapshot_size++, 
       *orders 
      }; 
      logger.log("%:% %() % %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        getCurrentTimeStr(&time_str), propagate_market_update.toString()
      ); 
      snapshot_socket.send(&propagate_market_update, sizeof(MDPMarketUpdate));
      snapshot_socket.sendAndRecv();  
     }
    }
  }
  const MDPMarketUpdate end_market_update { 
    snapshot_size++, 
    {MarketUpdateType::SNAPSHOT_END, last_increment_sequence_number}; 
  }; 
  logger.log("%:% %() % %\n", 
    __FILE__, __LINE__, __FUNCTION__, 
    getCurrentTimeStr(&time_str), end_market_update.toString()
  ); 
  snapshot_socket.send(&end_market_update, sizeof(MDPMarketUpdate));
  snapshot_socket.sendAndRecv(); 
  logger.log("%:% %() % Published snapshot of % orders.\n", 
    __FILE__, __LINE__, __FUNCTION__, 
    getCurrentTimeStr(&time_str), snapshot_size - 1
  );  
 }

 void SnapshotSynthesizer::run() { 
  logger.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str));
  while(is_running) { 
    for(auto market_update = snapshot_md_updates->getNextToRead(); snapshot_md_updates->size() &&  market_update; 
        market_update = snapshot_md_updates->getNextToRead()) { 
      logger.log("%:% %() % Processing %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        getCurrentTimeStr(&time_str),
        market_update->toString().c_str()
      );    
      addToSnapshot(market_update);
      snapshot_md_updates->updateReadIndex();      
    }
    if(getCurrentNanos() - last_snapshot_time > 60 * NANOS_TO_SECS) { 
      last_snapshot_time = getCurrentNanos(); 
      publishSnapshot(); 
    }
  }
 }
}