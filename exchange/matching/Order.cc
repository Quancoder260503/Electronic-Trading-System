#include "Order.hpp"

namespace Exchange {
  auto MatchingEngineOrder::toString() const -> std::string {
    std::stringstream ss;
    ss << "MatchingEngineOrder" << "["
       << "ticker:" << tickerIdToString(ticker_id) << " "
       << "cid:" << clientIdToString(client_id) << " "
       << "oid:" << orderIdToString(client_order_id) << " "
       << "moid:" << orderIdToString(market_order_id) << " "
       << "side:" << sideToString(side) << " "
       << "price:" << priceToString(price) << " "
       << "qty:" << quantityToString(quantity) << " "
       << "prio:" << priorityToString(priority) << " "
       << "prev:" << orderIdToString(prev_order ?
         prev_order_->market_order_id : OrderId_INVALID) << " "
       << "next:" << orderIdToString(next_order_ ? next_order_->market_order_id : OrderId_INVALID) 
       << "]";
    return ss.str();
  }
}