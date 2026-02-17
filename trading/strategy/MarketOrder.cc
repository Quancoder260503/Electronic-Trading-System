#include "MarketOrder.hpp"

namespace Trading { 
  auto MarketOrder::toString() const -> std::string { 
    std::stringstream ss;
    ss << "MarketOrder" << "["
       << "oid:" << orderIdToString(order_id) << " "
       << "side:" << sideToString(side) << " "
       << "price:" << priceToString(price) << " "
       << "qty:" << quantityToString(quantity) << " "
       << "prio:" << priorityToString(priority) << " "
       << "prev:" << orderIdToString(prev_order ?
           prev_order->order_id : ORDER_ID_INVALID) << " "
       << "next:" << orderIdToString(next_order ?
         next_order->order_id : ORDER_ID_INVALID) 
       << "]";
    return ss.str();
  }  

  auto MarketOrdersAtPrice::toString() const -> std::string { 
 std::stringstream ss;
      ss << "MarketOrdersAtPrice["
         << "side:" << sideToString(side) << " "
         << "price:" << priceToString(price) << " "
         << "first_mkt_order:" << (first_market_order ? first_market_order->toString() : "null") << " "
         << "prev:" << priceToString(prev_entry ? prev_entry->price : PRICE_INVALID) << " "
         << "next:" << priceToString(next_entry ? next_entry->price : PRICE_INVALID) << "]";
      return ss.str();
  }

  auto BBO::toString() const -> std::string { 
    std::stringstream ss;
      ss << "BBO{"
         << quantityToString(best_bid_quantity) << "@" <<
           priceToString(best_bid_price)
         << "X"
         << priceToString(best_ask_price) << "@" <<
            quantityToString(best_ask_quantity)
         << "}";
      return ss.str();
  }
}