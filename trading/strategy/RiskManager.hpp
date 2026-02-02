#pragma once 

#include "common/Macros.hpp"
#include "common/Logging.hpp"

#include "PositionKeeper.hpp"
#include "OMOrder.hpp"

using namespace Common; 

namespace Trading { 
  class OrderManager; 
  
  enum class RiskCheckResult : int8_t {
    INVALID = 0, 
    ORDER_LIMIT_EXCEEDED    = 1, 
    POSITION_LIMIT_EXCEEDED = 2,  
    LOSS_LIMIT_EXCEEDED     = 3,
    ALLOWED                 = 4  
  };
  inline auto riskCheckResultToString(RiskCheckResult result) -> std::string { 
    switch(result) { 
        case RiskCheckResult::INVALID:                 
          return "INVALID"; 
        case RiskCheckResult::ORDER_LIMIT_EXCEEDED:   
          return "ORDER_LIMIT_EXCEEDED"; 
        case RiskCheckResult::POSITION_LIMIT_EXCEEDED: 
          return "POSITION_LIMIT_EXCEEDED"; 
        case RiskCheckResult::LOSS_LIMIT_EXCEEDED:     
          return "LOSS_LIMIT_EXCEEDED"; 
        case RiskCheckResult::ALLOWED:                 
          return "ALLOWED"; 
        default:                                       
          return "UNKNOWN";
    }
  }
  struct RiskInfo { 
    const PositionInfo *position_info = nullptr; 
    RiskConfig risk_config; 

    auto checkPreTradeRisk(Side side, Quantity quantity) const noexcept { 
     if(UNLIKELY(quantity > risk_config.max_order_size)) { 
        return RiskCheckResult::ORDER_LIMIT_EXCEEDED; 
     }
     if(UNLIKELY(std::abs(position_info->position + sideToValue(side) * static_cast<int32_t>(quantity)) > 
        static_cast<int32_t> (risk_config.max_position))) {
        return RiskCheckResult::POSITION_LIMIT_EXCEEDED;  
     }
     if(UNLIKELY(position_info->total_pnl < risk_config.max_loss)) { 
        return RiskCheckResult::LOSS_LIMIT_EXCEEDED; 
     }
     return RiskCheckResult::ALLOWED; 
    }

    auto toString() const {
      std::stringstream ss;
      ss << "RiskInfo" << "["
         << "pos:" << position_info->toString() << " "
         << risk_config.toString()
         << "]";
      return ss.str();
    }
  }; 
  typedef std::array<RiskInfo, MATCHING_ENGINE_MAX_TICKERS> TickerRiskInfoHashMap; 

  class RiskManager { 
    public: 
      RiskManager(Common::Logger *logger_, const PositionKeeper *position_keeper, const TradeEngineConfigHashMap &ticker_cfg); 

      auto checkPreTradeRisk(TickerID ticker_id, Side side, Quantity quantity) const noexcept { 
        return ticker_risk.at(ticker_id).checkPreTradeRisk(side, quantity); 
      }

      RiskManager() = delete;

      RiskManager(const RiskManager &) = delete;

      RiskManager(const RiskManager &&) = delete;

      RiskManager &operator=(const RiskManager &) = delete;

      RiskManager &operator=(const RiskManager &&) = delete;
    private: 
      std::string time_str; 
      Common::Logger *logger = nullptr; 
      TickerRiskInfoHashMap ticker_risk;  
  }; 
}