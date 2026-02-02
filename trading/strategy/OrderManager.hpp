#pragma once 
#include "common/Macros.hpp"
#include "common/Logging.hpp"
#include "exchange/order_server/ClientResponse.hpp"
#include "exchange/order_server/ClientRequest.hpp"
#include "OMOrder.hpp"
#include "RiskManager.hpp"

using namespace Common; 

namespace Trading { 
  class TradeEngine; 
  class OrderManager { 
   public: 
    OrderManager(Common::Logger *logger_, TradeEngine *trade_engine_, RiskManager &risk_manager_) : 
    trade_engine(trade_engine_), risk_manager(risk_manager_),  logger(logger_) {}
    
    auto getOMOrderSideHashMap(TickerID ticker_id) noexcept { 
      return &(ticker_side_orders.at(ticker_id)); 
    }
    
    auto onOrderUpdate(const Exchange::MatchingEngineClientResponse *client_response) noexcept -> void; 

    auto newOrder(OMOrder *order, TickerID ticker_id, Price price, Side side, Quantity quantity) noexcept -> void;

    auto cancelOrder(OMOrder *order) noexcept -> void;

    auto moveOrder(OMOrder *order, TickerID ticker_id, Price price, Side side, Quantity quantity) noexcept; 

    auto moveOrders(TickerID ticker_id, Price bid_price, Price ask_price, Quantity clip) noexcept; 

    OrderManager() = delete;

    OrderManager(const OrderManager &) = delete;

    OrderManager(const OrderManager &&) = delete;

    OrderManager &operator=(const OrderManager &) = delete;

    OrderManager &operator=(const OrderManager &&) = delete;

   private: 
    TradeEngine *trade_engine = nullptr; 
    const RiskManager &risk_manager; 
    
    std::string time_str; 

    Common::Logger *logger = nullptr; 
    OMOrderTickerSideHashMap ticker_side_orders; 
    OrderID next_order_id = 1; 
  }; 
}