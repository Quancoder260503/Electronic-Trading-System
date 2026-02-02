#include "MarketOrderBook.hpp"

namespace Trading { 
 MarketOrderBook::MarketOrderBook(TickerID ticker_id_, Logger *logger_) : 
 ticker_id(ticker_id_), 
 orders_at_price_pool(MATCHING_ENGINE_MAX_PRICE_LEVELS), 
 order_pool(MATCHING_ENGINE_MAX_ORDER_IDS),
 logger(logger_) { }

 MarketOrderBook::~MarketOrderBook() { 
  logger->log("%:% %() % OrderBook\n%\n", 
    __FILE__, __LINE__, __FUNCTION__,
    Common::getCurrentTimeStr(&time_str),
    toString(false, true)
  );
  trade_engine = nullptr; 
  bids_by_price = asks_by_price = nullptr; 
  oid_to_order.fill(nullptr); 
 }

 auto MarketOrderBook::onMarketUpdate(const Exchange::MatchingEngineMarketUpdate *market_update) noexcept -> void { 
  const auto bid_updated = (bids_by_price && market_update->side == Side::BUY && market_update->price >= bids_by_price->price); 
  const auto ask_updated = (asks_by_price && market_update->side == Side::SELL && market_update->price <= asks_by_price->price); 
  switch(market_update->type) { 
    case Exchange::MarketUpdateType::ADD : { 
      ASSERT(
        oid_to_order.at(market_update->order_id) == nullptr, 
        "Add Market Order received for existing order id : " + std::to_string(market_update->order_id)
      ); 
      auto order = order_pool.allocate(
        market_update->order_id, 
        market_update->side, 
        market_update->price, 
        market_update->quantity, 
        market_update->priority, 
        nullptr, 
        nullptr
      ); 
      addOrder(order); 
    }
    break; 
    case Exchange::MarketUpdateType::MODIFY : { 
      auto order = oid_to_order.at(market_update->order_id); 
      ASSERT(order != nullptr, 
       "Modify Market Order received for non-existing order id : " + std::to_string(market_update->order_id)
      ); 
      order->quantity = market_update->quantity; 
    }
    break; 

    case Exchange::MarketUpdateType::CANCEL : { 
      auto order = oid_to_order.at(market_update->order_id); 
      ASSERT(order != nullptr, 
       "Cancel Market Order received for non-existing order id : " + std::to_string(market_update->order_id) 
      ); 
      removeOrder(order); 
    }
    break; 

    case Exchange::MarketUpdateType::TRADE : { 
      trade_engine->onTradeUpdate(market_update, this); 
      return;
    }
    break; 

    case Exchange::MarketUpdateType::CLEAR : { 
      for(auto &order : oid_to_order) { 
        if(order) { 
          order_pool.deallocate(order); 
        }
      }
      oid_to_order.fill(nullptr); 
      if(bids_by_price) { 
        for(auto bid = bids_by_price->next_entry; bid != bids_by_price; bid = bid->next_entry) { 
          orders_at_price_pool.deallocate(bid);   
        }
        orders_at_price_pool.deallocate(bids_by_price); 
      }
      if(asks_by_price) { 
        for(auto ask = asks_by_price->next_entry; ask != asks_by_price; ask = ask->next_entry) { 
          orders_at_price_pool.deallocate(ask); 
        }
        orders_at_price_pool.deallocate(asks_by_price); 
      }
      bids_by_price = asks_by_price = nullptr; 
    }
    break; 

    case Exchange::MarketUpdateType::INVALID : 
    case Exchange::MarketUpdateType::SNAPSHOT_START : 
    case Exchange::MarketUpdateType::SNAPSHOT_END : 
      break;
  }
  updateBBO(bid_updated, ask_updated); 
  logger->log("%:% %() % % %", 
    __FILE__, __LINE__, __FUNCTION__,
    Common::getCurrentTimeStr(&time_str), 
    market_update->toString(), 
    bbo.toString()
  );
  trade_engine->onOrderBookUpdate(market_update->ticker_id, market_update->price, market_update->side, this); 
  return; 
 }

 auto MarketOrderBook::updateBBO(bool bid_updated, bool ask_updated) noexcept -> void { 
  if(bid_updated) { 
    if(bids_by_price) { 
     bbo.best_bid_price = bids_by_price->price; 
     bbo.best_bid_quantity = bids_by_price->first_market_order->quantity; 
     for(auto order = bids_by_price->first_market_order->next_order; 
              order != bids_by_price->first_market_order; order = order->next_order) { 
      bbo.best_bid_quantity += order->quantity; 
     }
    } else { 
      bbo.best_bid_price    = PRICE_INVALID; 
      bbo.best_bid_quantity = QUANTITY_INVALID;   
    } 
  }
  if(ask_updated) { 
    if(asks_by_price) { 
     bbo.best_ask_price = asks_by_price->price; 
     bbo.best_ask_quantity = asks_by_price->first_market_order->quantity; 
     for(auto order = asks_by_price->first_market_order->next_order; 
              order != asks_by_price->first_market_order; order = order->next_order) { 
      bbo.best_ask_quantity += order->quantity; 
     }
    } else { 
      bbo.best_ask_price    = PRICE_INVALID; 
      bbo.best_ask_quantity = QUANTITY_INVALID;   
    } 
  }
 }

 auto MarketOrderBook::toString(bool detailed, bool validity_check) const -> std::string { 
  std::stringstream ss; 
  std::string curr_time_str; 
  auto printer = [&](std::stringstream &ss_, MarketOrdersAtPrice *itr, Side side, Price &last_price, bool sanity_check) { 
   char buffer[1 << 12]; 
   Quantity quantity = 0; 
   size_t num_orders = 0; 
   for(auto o_itr = itr->first_market_order; ; o_itr = o_itr->next_order) { 
    quantity += o_itr->quantity; 
    ++num_orders; 
    if(o_itr->next_order == itr->first_market_order) { 
      break; 
    }
   }
   sprintf(buffer, " <px:%3s p:%3s n:%3s> %-3s @ %-5s(%-4s)",
        priceToString(itr->price).c_str(), 
        priceToString(itr->prev_entry->price).c_str(), 
        priceToString(itr->next_entry->price).c_str(),
        priceToString(itr->price).c_str(), 
        quantityToString(quantity).c_str(), 
        std::to_string(num_orders).c_str()
   );
   ss_ << buffer;
   for(auto o_itr = itr->first_market_order; ; o_itr = o_itr->next_order) { 
    if (detailed) {
      sprintf(buffer, "[oid:%s q:%s p:%s n:%s] ",
              orderIdToString(o_itr->order_id).c_str(), 
              quantityToString(o_itr->quantity).c_str(),
              orderIdToString(o_itr->prev_order ? o_itr->prev_order->order_id : ORDER_ID_INVALID).c_str(),
              orderIdToString(o_itr->next_order ? o_itr->next_order->order_id : ORDER_ID_INVALID).c_str());
      ss_ << buffer;
    }
    if (o_itr->next_order == itr->first_market_order) { 
      break;
    }
   }
   ss_ << std::endl; 
  
   if(sanity_check) { 
    if((side == Side::SELL && last_price >= itr->price) || (side == Side::BUY && last_price <= itr->price)) { 
     FATAL("Bids/Asks not sorted by descending/ascending prices last : " + priceToString(last_price) + "itr : " + itr->toString());  
    }
    last_price = itr->price;
   }
  };
  ss << "Ticker : " << tickerIdToString(ticker_id) << std::endl; 
  
  { 
    auto ask_itr = asks_by_price; 
    auto last_ask_price = std::numeric_limits<Price>::min(); 
    for(size_t count = 0; ask_itr; count++) { 
     ss << "ASKS L : " << count << " => "; 
     auto next_ask_itr = (ask_itr->next_entry == asks_by_price ? nullptr : ask_itr->next_entry); 
     printer(ss, ask_itr, Side::SELL, last_ask_price, validity_check);
     ask_itr = next_ask_itr;  
    }
    ss << std::endl << "                          X" << std::endl << std::endl;
  } 
  
  {
    auto bid_itr = bids_by_price; 
    auto last_bid_price = std::numeric_limits<Price>::min(); 
    for(size_t count = 0; bid_itr; count++) { 
     ss << "BIDS L : " << count << " => "; 
     auto next_bid_itr = (bid_itr->next_entry == bids_by_price ? nullptr : bid_itr->next_entry); 
     printer(ss, bid_itr, Side::BUY, last_bid_price, validity_check);
     bid_itr = next_bid_itr;  
    }
  }
  return ss.str(); 
}

} 