#pragma once 

#include <array> 
#include <sstream> 
#include "common/Types.hpp"

using namespace  Common; 

namespace Trading { 
  struct MarketOrder { 
    OrderID order_id  = ORDER_ID_INVALID; 
    Side side         = Side::INVALID; 
    Price price       = PRICE_INVALID;
    Quantity quantity = QUANTITY_INVALID; 
    Priority priority = PRIORITY_INVALID; 
    MarketOrder *prev_order = nullptr; 
    MarketOrder *next_order = nullptr; 
    
    MarketOrder() = default; 
    MarketOrder(OrderID order_id_, Side side_, Price price_, 
                Quantity quantity_, Priority priority_, 
                MarketOrder *prev_order_, MarketOrder * next_order_) noexcept :
    order_id(order_id_), 
    side(side_), 
    price(price_), 
    quantity(quantity_), 
    priority(priority_), 
    prev_order(prev_order_), 
    next_order(next_order_) {} 
    
    auto toString() const -> std::string; 
  }; 
  typedef std::array<MarketOrder *, MATCHING_ENGINE_MAX_ORDER_IDS> OrderHashMap; 

  struct MarketOrdersAtPrice { 
    Side side = Side::INVALID; 
    Price price = PRICE_INVALID; 
    MarketOrder *first_market_order = nullptr; 
    MarketOrdersAtPrice *prev_entry = nullptr; 
    MarketOrdersAtPrice *next_entry = nullptr; 

    MarketOrdersAtPrice() = default; 
    MarketOrdersAtPrice(Side side_, Price price_, 
      MarketOrder *first_market_order_, 
      MarketOrdersAtPrice *prev_entry_, 
      MarketOrdersAtPrice *next_entry_) noexcept : 
      side(side_), 
      price(price_), 
      first_market_order(first_market_order_),
      prev_entry(prev_entry_),
      next_entry(next_entry_) 
      { }
    auto toString() const -> std::string; 
  }; 

  typedef std::array<MarketOrdersAtPrice *, MATCHING_ENGINE_MAX_PRICE_LEVELS> OrdersAtPriceHashMap;

  struct BBO { 
    Price best_bid_price = PRICE_INVALID; 
    Price best_ask_price = PRICE_INVALID; 
    Quantity best_bid_quantity = QUANTITY_INVALID; 
    Quantity best_ask_quantity = QUANTITY_INVALID; 
    auto toString() const -> std::string; 
  }; 
}