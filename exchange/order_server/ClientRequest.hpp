#pragma once
#include <sstream>

#include "common/LockFreeQueue.hpp"
#include "common/Types.hpp"

using namespace Common;

namespace Exchange {
#pragma pack(push, 1)  // optimize memory size
enum class ClientRequestType : uint8_t {
  INVALID = 0,
  NEW = 1,
  CANCEL = 2,
};
inline std::string clientRequestTypeToString(ClientRequestType type) {
  switch (type) {
  case ClientRequestType::NEW:
    return "NEW";
  case ClientRequestType::CANCEL:
    return "CANCEL";
  case ClientRequestType::INVALID:
    return "INVALID";
  }
  return "UNKNOWN";
}
struct MatchingEngineClientRequest {
  ClientRequestType type = ClientRequestType::INVALID;
  ClientID client_id = CLIENT_ID_INVALID;
  TickerID ticker_id = TICKER_ID_INVALID;
  OrderID order_id = ORDER_ID_INVALID;
  Side side = Side::INVALID;
  Price price = PRICE_INVALID;
  Quantity quantity = QUANTITY_INVALID;

  auto toString() const {
    std::stringstream ss;
    ss << "MatchingEngineClientRequest"
       << " ["
       << "type:" << clientRequestTypeToString(type)
       << " client:" << clientIdToString(client_id)
       << " ticker:" << tickerIdToString(ticker_id)
       << " oid:" << orderIdToString(order_id) << " side:" << sideToString(side)
       << " qty:" << quantityToString(quantity)
       << " price:" << priceToString(price) << "]";
    return ss.str();
  }
};
#pragma(pop)
typedef LockFreeQueue<MatchingEngineClientRequest> ClientRequestLFQueue;
}  // namespace Exchange