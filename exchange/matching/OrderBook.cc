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

auto MatchingEngineOrderBook::toString(bool detailed, bool validity_check) const -> std::string { 
  std::stringstream ss; 
  std::string time_str; 
  auto printer = [&](std::stringstream &ss, MatchingEngineOrderAtPrice *itr, Side side, Price &last_price, bool sanity_check) { 
   char buffer[1 << 12]; 
   Quantity quantity = 0; 
   size_t num_orders = 0; 
   for(auto o_itr = itr->first_order; ; o_itr = o_itr->next_order) { 
    quantity += o_itr->quantity; 
    ++num_orders; 
    if(o_itr->next_order == itr->first_order) { 
      break; 
    }
   }
   sprintf(buffer, " <px:%3s p:%3s n:%3s> %-3s @ %-5s(%-4s)",
        priceToString(itr->price).c_str(), 
        priceToString(itr->prev_entry->price).c_str(), 
        priceToString(itr->next_entry->price).c_str(),
        priceToString(itr->price).c_str(), 
        quantityToString(quantity).c_str(), 
        std::to_string(num_orders).c_str()
   );
   ss << buffer;
   for(auto o_itr = itr->first_order; ; o_itr = o_itr->next_order) { 
    if (detailed) {
      sprintf(buffer, "[oid:%s q:%s p:%s n:%s] ",
              orderIdToString(o_itr->market_order_id).c_str(), 
              quantityToString(o_itr->quantity).c_str(),
              orderIdToString(o_itr->prev_order ? o_itr->prev_order->market_order_id : OrderId_INVALID).c_str(),
              orderIdToString(o_itr->next_order ? o_itr->next_order->market_order_id : OrderId_INVALID).c_str());
      ss << buffer;
    }
    if (o_itr->next_order == itr->first_order) { 
      break;
    }
   }
   ss << std::endl; 
  
   if(sanity_check) { 
    if((side == Side::SELL && last_price >= itr->price) || (side == Side::BUY && last_price <= itr->price)) { 
     FATAL("Bids/Asks not sorted by descending/ascending prices last : " + priceToString(last_price) + "itr : " + itr->toString());  
    }
    last_price = itr->price;
   }
  };
  ss << "Ticker : " << tickerIdToString(ticker_id) << std::endl; 
  
  { 
    auto ask_itr = asks_by_price; 
    auto last_ask_price = std::numeric_limits<Price>::min(); 
    for(size_t count = 0; ask_itr; count++) { 
     ss << "ASKS L : " << count << " => "; 
     auto next_ask_itr = (ask_itr->next_entry == asks_by_price ? nullptr : ask_itr->next_entry); 
     printer(ss, ask_itr, Side::SELL, last_ask_price, validity_check);
     ask_itr = next_ask_itr;  
    }
    ss << std::endl << "                          X" << std::endl << std::endl;
  } 
  
  {
    auto bid_itr = bids_by_price; 
    auto last_bid_price = std::numeric_limits<Price>::min(); 
    for(size_t count = 0; ask_itr; count++) { 
     ss << "BIDS L : " << count << " => "; 
     auto next_bid_itr = (bid_itr->next_entry == bids_by_price ? nullptr : bid_itr->next_entry); 
     printer(ss, bid_itr, Side::BUY, last_bid_price, validity_check);
     bid_itr = next_bid_itr;  
    }
  }
  return ss.str(); 
}
}  // namespace Exchange
