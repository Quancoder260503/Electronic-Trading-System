#pragma once 

#include "common/Types.hpp"
#include "common/Mempool.hpp"
#include "common/Logging.hpp"
#include "MarketOrder.hpp"
#include "exchange/market_data/MarketUpdate.hpp"

namespace Trading { 
 class TradeEngine; 

 class MarketOrderBook final { 
  public: 
   MarketOrderBook(TickerID ticker_id_, Logger *logger); 
   ~MarketOrderBook(); 

   auto onMarketUpdate(const Exchange::MatchingEngineMarketUpdate *market_update) noexcept -> void; 

   auto setTradeEngine(TradeEngine *trade_engine_) -> void { 
    trade_engine = trade_engine_; 
   }

   auto updateBBO(bool bid_updated, bool ask_updated) noexcept -> void; 

   auto getBBO() const noexcept -> const BBO* { 
    return &bbo; 
   }

   auto toString(bool detailed, bool validity_check) const -> std::string; 

   MarketOrderBook() = delete;

   MarketOrderBook(const MarketOrderBook &) = delete;

   MarketOrderBook(const MarketOrderBook &&) = delete;

   MarketOrderBook &operator=(const MarketOrderBook &) = delete;

   MarketOrderBook &operator=(const MarketOrderBook &&) = delete;


  private:
   const TickerID ticker_id; 
   TradeEngine *trade_engine = nullptr; 
   OrderHashMap oid_to_order; 
   
   MemPool<MarketOrdersAtPrice>orders_at_price_pool; 
   MarketOrdersAtPrice *bids_by_price = nullptr; 
   MarketOrdersAtPrice *asks_by_price = nullptr; 
   OrdersAtPriceHashMap price_orders_at_price; 

   MemPool<MarketOrder>order_pool; 
   BBO bbo; 
   std::string time_str; 
   Logger *logger = nullptr; 

  private: 
   auto priceToIndex(Price price) const noexcept {
    return price % MATCHING_ENGINE_MAX_PRICE_LEVELS; 
   }

   auto getOrdersAtPrice(Price price) const noexcept -> MarketOrdersAtPrice* {
    return price_orders_at_price.at(priceToIndex(price));
  }

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

  auto removeOrder(MarketOrder* order) noexcept {
    auto order_at_price = getOrdersAtPrice(order->price);
    if (order->prev_order == order) {  // only one element in the list
      removeOrderAtPrice(order->side, order->price);
    } else {
      const auto order_before = order->prev_order;
      const auto order_after = order->next_order;
      order_before->next_order = order_after;
      order_after->prev_order = order_before;
      if (order_at_price->first_market_order == order) {
        order_at_price->first_market_order = order_after;
      }
      order->prev_order = order->next_order = nullptr;
    }
    oid_to_order.at(order->order_id) = nullptr; 
    order_pool.deallocate(order);
  }

  auto addOrderAtPrice(
    MarketOrdersAtPrice* new_orders_at_price) noexcept {
    price_orders_at_price.at(priceToIndex(new_orders_at_price->price)) =
        new_orders_at_price;

    const auto best_order_by_price =
        (new_orders_at_price->side == Side::BUY ? bids_by_price
                                                : asks_by_price);

    if (UNLIKELY(!best_order_by_price)) {
      (new_orders_at_price->side == Side::BUY ? bids_by_price : asks_by_price) =
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

  auto addOrder(MarketOrder* order) noexcept {
    const auto orders_at_price = getOrdersAtPrice(order->price);
    if (!orders_at_price) {
      order->next_order = order->prev_order = order;
      auto new_orders_at_price = orders_at_price_pool.allocate(
          order->side, order->price, order, nullptr, nullptr);
      addOrderAtPrice(new_orders_at_price);
    } else {
      auto first_order = orders_at_price->first_market_order;
      first_order->prev_order->next_order = order;
      order->prev_order = first_order->prev_order;
      order->next_order = first_order;
      first_order->prev_order = order;
    }
    oid_to_order.at(order->order_id) = order; 
  }
 }; 
 
 typedef std::array<MarketOrderBook *, MATCHING_ENGINE_MAX_TICKERS> MarketOrderBookHashMap; 

}