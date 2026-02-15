#include "OrderManager.hpp"
#include "TradeEngine.hpp"

namespace Trading { 
  auto OrderManager::newOrder(OMOrder *order, TickerID ticker_id, Price price, Side side, Quantity quantity) noexcept -> void { 
    const Exchange::MatchingEngineClientRequest new_request { 
      Exchange::ClientRequestType::NEW, 
      trade_engine->getClientID(), 
      ticker_id, 
      next_order_id, 
      side, 
      price, 
      quantity
    }; 
    trade_engine->sendClientRequest(&new_request); 
    (*order) = {ticker_id, next_order_id, side, price, quantity, OMOrderState::PENDING_NEW}; 
    next_order_id++; 
    logger->log("%:% %() % Sent new order % for %\n", 
      __FILE__, __LINE__, __FUNCTION__,
      Common::getCurrentTimeStr(&time_str),
      new_request.toString().c_str(), 
      order->toString().c_str()
    );
  }

  auto OrderManager::cancelOrder(OMOrder *order) noexcept -> void{ 
    const Exchange::MatchingEngineClientRequest cancel_request{
      Exchange::ClientRequestType::CANCEL,
      trade_engine->getClientID(), 
      order->ticker_id, 
      order->order_id,
      order->side, 
      order->price, 
      order->quantity
    }; 
    trade_engine->sendClientRequest(&cancel_request); 
    order->order_state = OMOrderState::PENDING_CANCEL; 
    logger->log("%:% %() % Sent cancel % for %\n",
      __FILE__, __LINE__, __FUNCTION__,
      Common::getCurrentTimeStr(&time_str),
      cancel_request.toString().c_str(), 
      order->toString().c_str()
    );
  }
 
  auto OrderManager::moveOrder(OMOrder *order, TickerID ticker_id, Price price, Side side, Quantity quantity) noexcept -> void {  
   switch (order->order_state) { 
    case OMOrderState::LIVE : { 
      if(order->price != price || order->quantity != quantity) { 
        cancelOrder(order); 
      }
    }
    break; 

    case OMOrderState::INVALID : 
    case OMOrderState::DEAD : {
      if(LIKELY(price != PRICE_INVALID)) { 
        const auto risk_result = risk_manager.checkPreTradeRisk(ticker_id, side, quantity); 
        if(LIKELY(risk_result == RiskCheckResult::ALLOWED)) { 
          newOrder(order, ticker_id, price, side, quantity); 
        } else { 
          logger->log("%:% %() % Ticker:% Side:% Qty:%RiskCheckResult:%\n", 
            __FILE__, __LINE__, __FUNCTION__,
            Common::getCurrentTimeStr(&time_str),
            tickerIdToString(ticker_id),
            sideToString(side),
            quantityToString(quantity),
            riskCheckResultToString(risk_result)
          );
        }
      }
    }
    break; 

    case OMOrderState::PENDING_NEW : 
    case OMOrderState::PENDING_CANCEL : { 

    }
    break; 
  } 
  }

  auto OrderManager::moveOrders(TickerID ticker_id, Price bid_price, Price ask_price, Quantity clip) noexcept -> void { 
   auto bid_order = &(ticker_side_orders.at(ticker_id).at(sideToIndex(Side::BUY))); 
   moveOrder(bid_order, ticker_id, bid_price, Side::BUY, clip); 
   auto ask_order = &(ticker_side_orders.at(ticker_id).at(sideToIndex(Side::SELL))); 
   moveOrder(ask_order, ticker_id, ask_price, Side::SELL, clip); 
  }

  auto OrderManager::onOrderUpdate(const Exchange::MatchingEngineClientResponse *client_response) noexcept -> void { 
    logger->log("%:% %() % %\n", 
                __FILE__, __LINE__, __FUNCTION__, 
                Common::getCurrentTimeStr(&time_str),
                client_response->toString().c_str()
    );
    auto order = &(ticker_side_orders.at(client_response->ticker_id).at(sideToIndex(client_response->side))); 
    logger->log("%:% %() % %\n", 
      __FILE__, __LINE__, __FUNCTION__, 
      Common::getCurrentTimeStr(&time_str),
      order->toString().c_str()
    );

    switch (client_response->type) { 
      case Exchange::ClientResponseType::ACCEPTED : { 
        order->order_state = OMOrderState::LIVE;
      }
      break; 

      case Exchange::ClientResponseType::CANCELLED : { 
        order->order_state = OMOrderState::DEAD;  
      }
      break; 

      case Exchange::ClientResponseType::FILLED : { 
        order->quantity = client_response->leaves_quantity; 
        if(!order->quantity) { 
          order->order_state = OMOrderState::DEAD; 
        }
      }
      break; 

      case Exchange::ClientResponseType::CANCEL_REJECTED :
      case Exchange::ClientResponseType::INVALID :  {

      }
      break; 
    }
  }

}

