#include "exchange/matching/Order.hpp"

namespace Exchange {
auto MatchingEngineOrder::toString() const -> std::string {
  std::stringstream ss;
  ss << "MatchingEngineOrder" << "[" << "ticker:" << tickerIdToString(ticker_id)
     << " "
     << "cid:" << clientIdToString(client_id) << " "
     << "oid:" << orderIdToString(client_order_id) << " "
     << "moid:" << orderIdToString(market_order_id) << " "
     << "side:" << sideToString(side) << " " << "price:" << priceToString(price)
     << " " << "qty:" << quantityToString(quantity) << " "
     << "prio:" << priorityToString(priority) << " "
     << "prev:"
     << orderIdToString(prev_order ? prev_order->market_order_id : ORDER_ID_INVALID)
     << " "
     << "next:"
     << orderIdToString(next_order ? next_order->market_order_id : ORDER_ID_INVALID)
     << "]";
  return ss.str();
}
}  // namespace Exchange