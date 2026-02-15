#include "RiskManager.hpp"
#include "OrderManager.hpp"

namespace Trading { 
 RiskManager::RiskManager(Common::Logger *logger_, 
    const PositionKeeper *position_keeper, const TradeEngineConfigHashMap &ticker_cfg) : logger(logger_) { 
  for(TickerID i = 0; i < MATCHING_ENGINE_MAX_TICKERS; i++) { 
    ticker_risk.at(i).position_info = position_keeper->getPositionInfo(i); 
    ticker_risk.at(i).risk_config   = ticker_cfg.at(i).risk_config; 
  }
 }
}