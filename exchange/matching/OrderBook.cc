#include "OrderBook.hpp"

#include "matching/MatchingEngine.hpp"

namespace Exchange {

MatchingEngineOrderBook::MatchingEngineOrderBook(
    TickerID ticker_id_, Logger* logger_, MatchingEngine* matching_engine_)
    : ticker_id(ticker_id_),
      logger(logger_),
      matching_engine(matching_engine_),
      orders_at_price_pool(MATCHING_ENGINE_MAX_PRICE_LEVELs),
      order_pool(MATCHING_ENGINE_MAX_ORDER_IDS) {}

MatchingEngineOrderBook::~MatchingEngineOrderBook() {
  logger->log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __FUNCTION__,
              Common::getCurrentTimeStr(&time_str_), toString(false, true));
  matching_engine = nullptr;
  bids_by_price = asks_by_price = nullptr;
  for (auto& itr : cid_oid_to_order) { itr.fill(nullptr); }
}

auto MatchingEngineOrderBook::match(TickerID ticker_id, ClientID client_id,
                                    Side side, OrderID client_order_id,
                                    OrderID new_market_order_id,
                                    MatchingEngineOrder* itr,
                                    Quantity* leaves_quantity) noexcept {
  auto order = itr;
  const auto order_quantity = order->quantity;
  const auto fill_quantity = std::min(order_quantity, *leaves_quantity);
  (*leaves_quantity) -= fill_quantity;
  order->quantity -= fill_quantity;
  client_response = {
      ClientResponseType::FILLED, client_id, ticker_id,  client_order_id,
      new_market_order_id,        side,      itr->price, fill_quantity,
      (*leaves_quantity)};
  matching_engine->sendClientResponse(&client_response);

  client_response = {ClientResponseType::FILLED,
                     order->client_id,
                     ticker_id,
                     order->client_order_id,
                     order->market_order_id,
                     order->side,
                     itr->priceq,
                     fill_quantity,
                     order->quantity};

  matching_engine->sendClientResponse(&client_response);
  if (!order->quantity) {
    market_update = {MarketUpdateType::CANCEL,
                     order->market_order_id,
                     ticker_id,
                     order->side,
                     order->price,
                     order_quantity,
                     PRIORITY_INVALID};
    matching_engine->sendMarketUpdate(&market_update);
    removeOrder(order);
  } else {
    market_update = {MarketUpdateType::MODIFY,
                     order->market_order_id,
                     ticker_id,
                     order->side,
                     order->price,
                     order->quantity,
                     order->priority};
    matching_engine->sendMarketUpdate(&market_update);
  }
}

auto MatchingEngineOrderBook::checkForMatch(
    OrderID client_id, OrderID client_order_id, TickerID ticker_id, Side side,
    Price price, Quantity quantity, Quantity new_market_order_id) noexcept {
  auto leaves_quantity = quantity;
  if (side == Side::BUY) {
    while (leaves_quantity && asks_by_price) {
      const auto ask_itr = asks_by_price->first_order;
      if (LIKELY(price < ask_itr->price)) { break; }
      match(ticker_id, client_id, side, client_order_id, new_market_order_id,
            ask_itr, &leaves_quantity);
    }
  }
  if (side == Side::SELL) {
    while (leaves_quantity && bids_by_price) {
      const auto bid_itr = bids_by_price->first_order;
      if (LIKELY(price > bid_itr->price)) { break; }
      match(ticker_id, client_id, side, client_order_id, new_market_order_id,
            bid_itr, &leaves_quantity);
    }
  }
  return leaves_quantity;
}

auto MatchingEngineOrderBook::add(ClientID client_id, OrderID client_order_id,
                                  TickerID ticker_id, Side side, Price price,
                                  Quantity quantity) noexcept -> void {
  const auto new_market_order_id = generateNewMarketOrderId();
  client_response = {ClientResponseType::ACCEPTED,
                     ticker_id,
                     client_id,
                     client_order_id,
                     new_market_order_id,
                     side,
                     price,
                     0,
                     quantity};
  matching_engine->sendClientResponse(&client_response);
  const auto leaves_quantity =
      checkForMatch(client_id, client_order_id, ticker_id, side, price,
                    quantity, new_market_order_id);

  if (LIKELY(leaves_quantity)) {
    const auto priority = getNextPriority(ticker_id, price);
    auto order = order_pool.allocate(ticker_id, client_id, client_order_id,
                                     new_market_order_id, side, price,
                                     leaves_quantity, nullptr, nullptr);

    addOrder(order);  // actually do the job
    market_update = {MarketUpdateType::ADD,
                     new_market_order_id,
                     ticker_id,
                     side,
                     price,
                     leaves_quantity,
                     priority};
    matching_engine->sendMarketUpdate(&market_update);
  }
}

auto MatchingEngineOrderBook::cancel(ClientID client_id, OrderID order_id,
                                     TickerID ticker_id) noexcept -> void {
  auto is_cancelable = (client_id < cid_oid_to_order.size());
  MatchingEngineOrder* exchange_order = nullptr;
  if (LIKELY(is_cancelable)) {
    auto& co_itr = cid_oid_to_order.at(client_id);
    exchange_order = co_itr.at(order_id);
    is_cancelable = (exchange_order != nullptr);
  }
  if (UNLIKELY(!is_cancelable)) {
    client_response = {ClientResponseType::CANCEL_REJECTED,
                       client_id,
                       ticker_id,
                       order_id,
                       ORDER_ID_INVALID,
                       Side::INVALID,
                       PRICE_INVALID,
                       QUANTITY_INVALID,
                       QUANTITY_INVALID};
  } else {
    client_response = {ClientResponseType::CANCELLED,
                       client_id,
                       ticker_id,
                       order_id,
                       exchange_order->market_order_id,
                       exchange_order->side,
                       exchange_order->price,
                       QUANTITY_INVALID,
                       exchange_order->quantity};

    market_update = {
        MarketUpdateType::CANCEL, exchange_order->market_order_id, ticker_id,
        exchange_order->side,     exchange_order->price,           0,
        exchange_order->priority,
    };
    removeOrder(exchange_order);
    matching_engine->sendMarketUpdate(&market_update);
  }
  matching_engine->sendClientResponse(&client_response);
}

}  // namespace Exchange
