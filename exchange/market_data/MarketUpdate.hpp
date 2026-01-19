#pragma once
#include <sstream>

#include "common/LockFreeQueue.hpp"
#include "common/Types.hpp"

using namespace Common;

namespace Exchange {
#pragma pack(push, 1)  // optimize memory size
enum class MarketUpdateType : uint8_t {
  INVALID = 0,
  ADD = 1,
  MODIFY = 2,
  CANCEL = 3,
  TRADE = 4
};
inline std::string MarketUpdateType(ClientRequestType type) {
  switch (type) {
  case MarketUpdateType::ADD:
    return "ADD";
  case MarketUpdateType::MODIFY:
    return "MODIFY";
  case MarketUpdateType::CANCEL:
    return "CANCEL";
  case MarketUpdateType::TRADE:
    return "TRADE";
  case MarketUpdateType::INVALID:
    return "INVALID";
  }
  return "UNKNOWN";
}

struct MatchingEngineMarketUpdate {
  MarketUpdateType type = MarketUpdateType::INVALID;
  OrderID order_id = ORDER_ID_INVALID;
  TickerID ticker_id = TICKER_ID_INVALID;
  Side side = Side::INVALID;
  Price price = PRICE_INVALID;
  Quantity quantity = QUANTITY_INVALID;
  Priority priority = PRIORITY_INVALID;

  auto toString() const {
    std::stringstream ss;
    ss << "MatchingEngineMarketUpdate"
       << " ["
       << " type:" << marketUpdateTypeToString(type)
       << " ticker:" << tickerIdToString(ticker_id)
       << " oid:" << orderIdToString(order_id) << " side:" << sideToString(side)
       << " qty:" << quantityToString(quantity)
       << " price:" << priceToString(price)
       << " priority:" << priorityToString(priority) << "]";
    return ss.str();
  }
};
#pragma(pop)
typedef LockFreeQueue<MatchingEngineMarketUpdate> MarketUpdateLFQueue;
}  // namespace Exchange