#pragma once 
#include <array> 
#include <sstream> 
#include "common/Types.hpp"

using namespace Common; 

namespace Trading { 
  enum class OMOrderState : uint8_t { 
    INVALID = 0,
    PENDING_NEW = 1, 
    LIVE = 2, 
    PENDING_CANCEL = 3, 
    DEAD = 4 
  }; 
  inline auto OMOrderStateToString(OMOrderState side) -> std::string {
    switch(side) { 
     case OMOrderState::INVALID : 
       return "INVALID"; 
     case OMOrderState::PENDING_NEW : 
       return "PENDING_NEW"; 
     case OMOrderState::LIVE: 
       return "LIVE"; 
     case OMOrderState::PENDING_CANCEL:
       return "PENDING_CANCEL"; 
      case OMOrderState::DEAD : 
       return "DEAD";    
    }
    return "UNKNOWN"; 
  }

  struct OMOrder { 
   TickerID ticker_id = TICKER_ID_INVALID; 
   OrderID  order_id  = ORDER_ID_INVALID; 
   Side side          = Side::INVALID; 
   Price price        = PRICE_INVALID;
   Quantity quantity  = QUANTITY_INVALID; 
   OMOrderState order_state = OMOrderState::INVALID; 
   auto toString() const {
      std::stringstream ss;
      ss << "OMOrder" << "["
         << "tid:" << tickerIdToString(ticker_id) << " "
         << "oid:" << orderIdToString(order_id) << " "
         << "side:" << sideToString(side) << " "
         << "price:" << priceToString(price) << " "
         << "qty:" << quantityToString(quantity) << " "
         << "state:" << OMOrderStateToString(order_state)
         << "]";
      return ss.str();
    } 
  }; 
  typedef std::array<OMOrder, sideToIndex(Side::MAX)>OMOrderSideHashMap; 
  typedef std::array<OMOrderSideHashMap, MATCHING_ENGINE_MAX_TICKERS>OMOrderTickerSideHashMap; 
}