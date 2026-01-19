#pragma once
#include "Order.hpp"
#include "common/Logging.hpp"
#include "common/Mempool.hpp"
#include "common/Types.hpp"
#include "market_data/MarketUpdate.hpp"
#include "order_server/ClientRequest.hpp"
#include "order_server/ClientResponse.hpp"

using namespace Common;

namespace Exchange {
class MatchingEngine;
class MatchingEngineOrderBook final {
  explicit MatchingEngineOrderBook(TickerID ticket_id_, Logger* logger_,
                                   MatchingEngine* matching_engine_);

  ~MatchingEngineOrderBook();

  auto add(ClientID client_id, OrderID client_order_id, TickerID ticker_id,
           Side side, Price price, Quantity quantity) noexcept -> void;

  auto cancel(ClientID client_id, OrderID order_id, TickerID ticker_id) noexcept
      -> void;

  auto toString(bool detailed, bool validity_check) const -> std::string;

  MatchingEngineOrderBook() = delete;
  MatchingEngineOrderBook(const MatchingEngineOrderBook&) = delete;
  MatchingEngineOrderBook(const MatchingEngineOrderBook&&) = delete;

  MatchingEngineOrderBook& operator=(const MatchingEngineOrderBook&) = delete;
  MatchingEngineOrderBook& operator=(const MatchingEngineOrderBook&&) = delete;

private:
  TickerID ticker_id = TICKER_ID_INVALID;

  MatchingEngine* matching_engine = nullptr;

  ClientOrderHashMap cid_oid_to_order;

  MemPool<MatchingEnginerOrderAtPrice> orders_at_price_pool;

  MatchingEngineOrderAtPrice* bids_by_price = nullptr;
  MatchingEngineOrderAtPrice* asks_by_price = nullptr;

  OrdersAtPriceHashMap price_orders_at_price;

  MemPool<MatchingEngineOrder> order_pool;

  MatchingEngineClientResponse client_response;
  MatchingEngineMarketUpdate market_update;

  OrderID next_market_order_id = 1;

  std::string time_str;
  Logger* logger = nullptr;

private:
  auto generateNewMarketOrderId() noexcept -> OrderID {
    return next_market_order_id++;
  }

  auto priceToIndex(Price price) const noexcept {
    return price % MATCHING_ENGINE_MAX_PRICE_LEVELs;
  }

  auto getOrdersAtPrice(Price price) const noexcept
      -> MatchingEngineOrderAtPrice* {
    return price_orders_at_price.at(priceToIndex(price));
  }

  auto getNextPriority(Price price) noexcept {
    const auto orders_at_price = getOrdersAtPrice(price);
    if (!orders_at_price) { return 1; }
    return orders_at_price->first_order->prev_order->priority + 1;
  }

  auto match(TickerID ticker_id, ClientID client_id, Side side,
             OrderID client_order_id, OrderID new_market_order_id,
             MatchingEngineOrder* itr, Quantity* leaves_quantity) noexcept;

  auto checkForMatch(OrderID client_id, OrderID client_order_id,
                     TickerID ticker_id, Side side, Price price,
                     Quantity quantity, Quantity new_market_order_id) noexcept;

  auto removeOrderAtPrice(Side side, Price price) noexcept {
    const auto best_orders_by_price =
        (side == Side::BUY ? bids_by_price : asks_by_price);
    auto orders_at_price = getOrdersAtPrice(price);
    if (UNLIKELY(orders_at_price->next_entry == orders_at_price)) {
      (side == Side::BUY ? bids_by_price : asks_by_price) = nullptr;
    } else {
      orders_at_price->prev_entry->next_entry = orders_at_price->next_entry;
      orders_at_price->next_entry->prev_entry = orders_at_price->prev_entry;
      if (orders_at_price == best_orders_by_price) {
        (side == Side::BUY ? bids_by_price : asks_by_price) =
            orders_at_price->next_entry;
      }
      orders_at_price->prev_entry = orders_at_price->next_entry = nullptr;
    }
    price_orders_at_price.at(priceToIndex(price)) = nullptr;
    orders_at_price_pool.deallocate(orders_at_price);
  }

  auto removeOrder(MatchingEngineOrder* order) noexcept {
    auto order_at_price = getOrdersAtPrice(order->price);
    if (order->prev_order == order) {  // only one element in the list
      removeOrderAtPrice(order->side, order->price);
    } else {
      const auto order_before = order->prev_order;
      const auto order_after = order->next_order;
      order_before->next_order = order_after;
      order_after->prev_order = order_before;
      if (order_at_price->first_order == order) {
        order_at_price->first_order = order_after;
      }
      order->prev_order = order->next_order = nullptr;
    }
    cid_oid_to_order.at(order->client_id).at(order->client_order_id) = nullptr;
    order_pool.deallocate(order);
  }

  auto addOrderAtPrice(
      MatchingEngineOrderAtPrice* new_orders_at_price) noexcept {
    price_orders_at_price.at(priceToIndex(new_orders_at_price->price)) =
        new_orders_at_price;

    const auto best_order_by_price =
        (new_orders_at_price->side == Side::BUY ? bids_by_price
                                                : asks_by_price);

    if (UNLIKELY(!best_order_by_price)) {
      (new_orders_at_price->sice == Side::BUY ? bids_by_price : asks_by_price) =
          new_orders_at_price;
      new_orders_at_price->prev_entry = new_orders_at_price->next_entry =
          new_orders_at_price;
    } else {
      auto target = best_order_by_price;
      bool add_after = ((new_orders_at_price->side == Side::SELL &&
                         new_orders_at_price->price > target->price) ||
                        (new_orders_at_price->side == Side::BUY &&
                         new_orders_at_price->price < target->price));
      if (add_after) {
        target = target->next_entry;
        add_after = ((new_orders_at_price->side == Side::SELL &&
                      new_orders_at_price->price > target->price) ||
                     (new_orders_at_price->side == Side::BUY &&
                      new_orders_at_price->price < target->price));
      }
      while (add_after && target != best_order_by_price) {
        add_after = ((new_orders_at_price->side == Side::SELL &&
                      new_orders_at_price->price > target->price) ||
                     (new_orders_at_price->side == Side::BUY &&
                      new_orders_at_price->price < target->price));
        if (add_after) { target = target->next_entry; }
      }
      if (add_after) {
        target = best_order_by_price->prev_entry;
        new_orders_at_price->prev_entry = target;
        target->next_entry->prev_entry = new_orders_at_price;
        new_orders_at_price->next_entry = target->next_entry;
        target->next_entry = new_orders_at_price;

      } else {
        new_orders_at_price->prev_entry = target->prev_entry;
        new_orders_at_price->next_entry = target;
        target->prev_entry->next_entry = new_orders_at_price;
        target->prev_entry = new_orders_at_price;

        if ((new_orders_at_price->side == Side::BUY &&
             new_orders_at_price->price > best_order_by_price->price) ||
            (new_orders_at_price->side == Side::SELL &&
             new_orders_at_price->price < best_order_by_price->price)) {
          target->next_entry =
              (target->next_entry == best_order_by_price ? new_orders_at_price
                                                         : target->next_entry);
          (new_orders_at_price->side == Side::BUY ? bids_by_price
                                                  : asks_by_price) =
              new_orders_at_price;
        }
      }
    }
  }

  auto addOrder(MatchingEngineOrder* order) noexcept {
    const auto orders_at_price = getOrdersAtPrice(order->price);
    if (!orders_at_price) {
      order->next_order = order->prev_order = order;
      auto new_orders_at_price = orders_at_price_pool.allocate(
          order->side, order->price, order, nullptr, nullptr);
      addOrderAtPrice(new_orders_at_price);
    } else {
      auto first_order = orders_at_price->first_order;
      first_order->prev_order->next_order = order;
      order->prev_order = first_order->prev_order;
      order->next_order = first_order;
      first_order->prev_order = order;
    }
    cid_oid_to_order.at(order->client_id).at(order->client_order_id) = order;
  }
};
typedef std::array<MatchingEngineOrderBook*, MATCHING_ENGINE_MAX_TICKERS>
    OrderBookHashMap;
}  // namespace Exchange