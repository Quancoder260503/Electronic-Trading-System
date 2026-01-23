#include "MarketDataPublisher.hpp"
#include "common/Types.hpp
#include "common/Macros.hpp"
#include <string> 

namespace Exchange {
 MarketDataPublisher::MarketDataPublisher(
    MarketUpdateLFQueue *market_updates, const std::string &iface, 
    const std::string &snapshot_ip, int snapshot_port, 
    const std::string &incremental_ip, int incremental_port) : 
 outgoing_market_updates(market_updates), 
 snapshot_market_updates(MATCHING_ENGINE_MAX_MARKET_UPDATES), 
 is_running(false), 
 logger("exchange_market_data_publisher.log"), 
 incremental_socket(logger) 
 { 
  ASSERT(incremental_socket.init(incremental_ip, iface, incremental_port, false) >= 0, 
    "Unable to create incremental mcast socket. error : " + std::string(std::strerror(errno))); 
  snapshot_synthesizer = new SnapshotSynthesizer()
 }
 
 MarketDataPublisher::~MarketDataPublisher() { 
  
 }

 MarketDataPublisher::start() { 

 }
 MarketDataPublisher::stop() { 

 }
 MarketDataPublisher::run() { 

 }
} 
