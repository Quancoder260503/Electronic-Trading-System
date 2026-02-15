#pragma once 
#include "common/Types.hpp"
#include "common/Macros.hpp"
#include "common/Logging.hpp"
#include "exchange/order_server/ClientResponse.hpp"
#include "MarketOrderBook.hpp"

using namespace Common; 

namespace Trading { 
 struct PositionInfo { 
  int32_t position; 
  double real_pnl = 0, unreal_pnl = 0, total_pnl = 0; 
  //  Tracks the product ofprice and execution quantity on each side when there   
  // are open long or short positions.  
  std::array<double, sideToIndex(Side::MAX) + 1> open_vwap; 
  Quantity volume = 0; // track the quantity executed 
  const BBO *bbo = nullptr; 

  auto toString() const -> std::string { 
    std::stringstream ss;
    ss << "Position{"
       << "pos:" << position
       << " u-pnl:" << unreal_pnl
       << " r-pnl:" << real_pnl
       << " t-pnl:" << total_pnl
       << " vol:" << quantityToString(volume)
       << " vwaps:[" << (position ? open_vwap.at(sideToIndex(Side::BUY)) / std::abs(position) : 0)
       << "X" << (position ? open_vwap.at(sideToIndex(Side::SELL)) / std::abs(position) : 0)
       << "] "
       << (bbo ? bbo->toString() : "") 
       << "}";
    return ss.str();
  }

  auto addFill(const Exchange::MatchingEngineClientResponse *client_response, Logger *logger) noexcept { 
   const auto old_position = position; 
   const auto side_index   = sideToIndex(client_response->side); 
   const auto opp_side_index = sideToIndex(
     client_response->side == Side::BUY ? Side::SELL : Side::BUY
   ); 
   const auto side_value = sideToValue(client_response->side); 
   position += client_response->exec_quantity * side_value; 
   volume   += client_response->exec_quantity; 
   if(old_position * side_value >= 0) { 
    open_vwap[side_index] += static_cast<double>(client_response->price * client_response->exec_quantity); 
   } else { 
    const auto opp_side_vwap = open_vwap[opp_side_index] / std::abs(old_position); 
    real_pnl += std::min(static_cast<int32_t>(client_response->exec_quantity), std::abs(old_position)) *
                (opp_side_vwap - static_cast<double>(client_response->price)) * side_value;
    if(position * old_position < 0) { // flipped to opposite side 
       open_vwap[side_index] = static_cast<double>(client_response->price * std::abs(position)); 
       open_vwap[opp_side_index] = 0; 
    }
   }
   if(!position) { 
    open_vwap[sideToIndex(Side::BUY)] = open_vwap[sideToIndex(Side::SELL)] = 0; 
    unreal_pnl = 0; 
   } else { 
    if(position > 0) { 
      unreal_pnl = (static_cast<double>(client_response->price) - open_vwap[sideToIndex(Side::BUY)] / std::abs(position)) * std::abs(position); 
    } else { 
      unreal_pnl = (open_vwap[sideToIndex(Side::SELL)] / std::abs(position) - static_cast<double>(client_response->price)) * std::abs(position); 
    }
   }
   total_pnl = unreal_pnl + real_pnl; 
   std::string time_str; 
   logger->log("%:% %() % % %\n", 
    __FILE__, __LINE__, __FUNCTION__, 
    Common::getCurrentTimeStr(&time_str),
    toString(), 
    client_response->toString().c_str()
   );
  }

  auto updateBBO(const BBO *bbo_, Logger *logger) noexcept { 
    std::string time_str; 
    bbo = bbo_; 
    if(position && bbo->best_bid_price != PRICE_INVALID && bbo->best_ask_price != PRICE_INVALID) { 
      const auto mid_price = (bbo->best_bid_price + bbo->best_ask_price) * 0.5; 
      if(position > 0) { 
        unreal_pnl = (mid_price - open_vwap[sideToIndex(Side::BUY)] / std::abs(position)) * std::abs(position); 
      }  else { 
        unreal_pnl = (open_vwap[sideToIndex(Side::SELL)] / std::abs(position) - mid_price) * std::abs(position); 
      }
    }
    const auto old_total_pnl = total_pnl; 
    total_pnl = unreal_pnl + real_pnl; 
    if(total_pnl != old_total_pnl) { 
      logger->log("%:% %() % % %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str),
        toString(), 
        bbo_->toString()
      );
    }
  }
 };
 
 class PositionKeeper { 
  public:
    PositionKeeper(Common::Logger *logger_) : logger(logger_) {}
    
    auto addFill(const Exchange::MatchingEngineClientResponse *client_response) noexcept { 
      ticker_position.at(client_response->ticker_id).addFill(client_response, logger); 
    }

    auto updateBBO(TickerID ticker_id, const BBO *bbo) noexcept { 
      ticker_position.at(ticker_id).updateBBO(bbo, logger); 
    }

    auto getPositionInfo(TickerID ticker_id) const noexcept { 
     return &(ticker_position.at(ticker_id)); 
    }

    auto toString() const {
      double total_pnl = 0; 
      Quantity total_volume = 0; 
      std::stringstream ss; 
      for(TickerID ticker_id = 0; ticker_id < ticker_position.size(); ticker_id++) { 
        ss << "TickerId:" << tickerIdToString(ticker_id) << " " << ticker_position.at(ticker_id).toString() << "\n";
        total_pnl   += ticker_position.at(ticker_id).total_pnl; 
        total_volume += ticker_position.at(ticker_id).volume; 
      }
      ss << "Total PnL:" << total_pnl << " Vol:" << total_volume << "\n";
      return ss.str();
    }

    PositionKeeper() = delete;

    PositionKeeper(const PositionKeeper &) = delete;

    PositionKeeper(const PositionKeeper &&) = delete;

    PositionKeeper &operator=(const PositionKeeper &) = delete;

    PositionKeeper &operator=(const PositionKeeper &&) = delete;

  private:
    std::string time_str; 
    Common::Logger *logger = nullptr; 
    std::array<PositionInfo, MATCHING_ENGINE_MAX_TICKERS> ticker_position; 
 }; 
}