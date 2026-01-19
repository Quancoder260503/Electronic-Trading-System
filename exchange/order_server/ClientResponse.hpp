#pragma once
#include <sstream>

#include "common/LockFreeQueue.hpp"
#include "common/Types.hpp"

using namespace Common;

namespace Exchange {
#pragma pack(push, 1)  // optimize memory size
enum class ClientResponseType : uint8_t {
  INVALID = 0,
  ACCEPTED = 1,
  CANCELLED = 2,
  FILLED = 3,
  CANCEL_REJECTED = 4
};
inline std::string clientResponseTypeToString(ClientResponseType type) {
  switch (type) {
  case ClientResponseType::INVALID:
    return "INVALID";
  case ClientResponseType::ACCEPTED:
    return "ACCEPTED";
  case ClientResponseType::CANCELLED:
    return "CANCELLED";
  case ClientResponseType::FILLED:
    return "FILLED";
  case ClientResponseType::CANCEL_REJECTED:
    return "CANCEL_REJECTED";
  }
  return "UNKNOWN";
}

struct MatchingEngineClientResponse {
  ClientResponseType type = ClientResponseType::INVALID;
  ClientID client_id = CLIENT_ID_INVALID;
  TickerID ticker_id = TICKER_ID_INVALID;
  OrderID client_order_id = ORDER_ID_INVALID;
  OrderID market_order_id = ORDER_ID_INVALID;
  Side side = Side::INVALID;
  Price price = PRICE_INVALID;
  Quantity exec_quantity = QUANTITY_INVALID;
  Quantity leaves_quantity = QUANTITY_INVALID;

  auto toString() const {
    std::stringstream ss;
    ss << "MatchingEngineClientResponse"
       << " ["
       << "type:" << clientResponseTypeToString(type)
       << " client:" << clientIdToString(client_id)
       << " ticker:" << tickerIdToString(ticker_id)
       << " coid:" << orderIdToString(client_order_id)
       << " moid:" << orderIdToString(market_order_id)
       << " side:" << sideToString(side)
       << " exec_qty:" << quantityToString(exec_quantity)
       << " leaves_qty:" << quantityToString(leaves_quantity)
       << " price:" << priceToString(price) << "]";
    return ss.str();
  }
};
#pragma(pop)
typedef LockFreeQueue<MatchingEngineClientResponse> ClientResponseLFQueue;
}  // namespace Exchange