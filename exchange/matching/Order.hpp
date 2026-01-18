#pragma once 
#include <array> 
#include <sstream> 
#include "common/Types.hpp"

namespace Exchange {
  struct MatchingEngineOrder { 
    TickerID ticker_id = TICKER_ID_INVALID; 
    ClientID client_id = CLIENT_ID_INVALID; 
    OrderID  client_order_id  = ORDER_ID_INVALID;
    OrderID  market_order_id  = ORDER_ID_INVALID; 
    Side side = Side::INVALID; 
    Price price = PRICE_INVALID; 
    Quantity quantity = QUANTITY_INVALID; 
    Priority priority = PRIORITY_INVALID; 

    MatchingEngineOrder *prev_order = nullptr; 
    MatchingEngineOrder *next_order = nullptr; 

    MatchingEngineOrder() = default; 
    MatchingEngineOrder(TickerID ticker_id_, ClientID client_id_, 
                       OrderID client_order_id_, OrderID market_order_id_, 
                       Side side_, Price price_, Quantity quantity_, Priority priority_
                       MatchingEnginerOrder *prev_order_,
                       MatchingEngineOrder  *next_order_) noexcept : 
                       ticket_id(ticket_id_), 
                       client_id(client_id_), 
                       client_order_id(client_order_id_), 
                       market_order_id(market_order_id_),
                       side(side_), 
                       price(price_), 
                       quantity(quantity_), 
                       priority(priority_), 
                       prev_order(prev_order_), 
                       next_order(next_order_) { 
                    
    }

    auto toString() const -> std::string; 
  }; 
  
  typedef std::array<MatchingEngineOrder*, MATCHING_ENGINE_MAX_ORDER_IDS> OrderHashMap; 
  typedef std::array<OrderHashMap, MATCHING_ENGINE_MAX_NUM_CLIENTS> ClientOrderHashMap; 
  
  struct MatchingEnginerOrderAtPrice { 
    Side side = Side::INVALID; 
    Price price = PRICE_INVALID; 
    MatchingEngineOrder *first_order = nullptr; 
    MatchingEnginerOrderAtPrice *prev_entry = nullptr; 
    MatchingEnginerOrderAtPrice *next_entry = nullptr; 

    MatchingEnginerOrderAtPrice(Side side_, Price price_, 
    MatchingEngineOrder *first_order_, 
    MatchingEnginerOrderAtPrice *prev_entry_, 
    MatchingEnginerOrderAtPrice *next_entry_) : 
    side(side_), price(price_), first_order(first_order_),
    next_entry(next_entry_), prev_entry(prev_entry_) {

    }

    auto toString() const {
      std::stringstream ss;
      ss << "MatchingEngineOrdersAtPrice["
         << "side:" << sideToString(side) << " "
         << "price:" << priceToString(price) << " "
         << "first_me_order:" << (first_me_order ? first_me_order->toString() : "null") << " "
         << "prev:" << priceToString(prev_entry ? prev_entry->price : Price_INVALID) << " "
         << "next:" << priceToString(next_entry ? next_entry->price_ : Price_INVALID) << "]";
      return ss.str();
    }
  };
  typedef std::array<MatchingEnginerOrderAtPrice*, MATCHING_ENGINE_MAX_PRICE_LEVELS> OrdersAtPriceHashMap; 
}