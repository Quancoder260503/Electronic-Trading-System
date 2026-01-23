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
  snapshot_synthesizer = new SnapshotSynthesizer(&snapshot_market_updates, )
 }
 
 MarketDataPublisher::~MarketDataPublisher() { 
  stop(); 
  using namespace std::literals::chrono_literals; 
  std::this_thread::sleep_for(5s);
  delete snapshot_synthesizer; 
  snapshot_synthesizer = nullptr; 
 }

 MarketDataPublisher::start() { 
  is_running = true; 
  ASSERT(Common::createAndStartThread(-1, "Exchange/MarketDataPublisher", [this]() { 
   run(); }) != nullptr, "Failed to start Market Data thread."); 
  snapshot_synthesizer->start(); 

 }
 MarketDataPublisher::stop() { 
  is_running = false; 
  snapshot_synthesizer->stop(); 
 }

 MarketDataPublisher::run() noexcept -> void {
  logger.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  while(is_running) { 
   for(auto market_update = outgoing_market_updates->getNextToRead(); outgoing_market_updates->size() && market_update; 
      market_update = out_going_market_updates->getNextToRead()) { 
    logger.log("%:% %() % Sending seq:% %\n", 
      __FILE__, __LINE__, __FUNCTION__, 
      Common::getCurrentTimeStr(&time_str), 
      next_increment_sequence_number,
      market_update->toString().c_str()
    );

    incremental_socket.send(&next_increment_sequence_number, sizeof(next_increment_sequence_number)); 
    incremental_socket.send(market_update, sizeof(MatchingEngineMarketUpdate)); 

    outgoing_market_updates->updateReadIndex(); 

    auto next_write = snapshot_market_updates.getNextToWrite(); 
    next_write->sequence_number = next_increment_sequence_number; 
    next_write->me_market_update = *market_update;
    snapshot_market_updates.updateWriteIndex(); 
    ++next_incremental_sequence_number; 
   }
   incremental_socket.sendAndRecv(); 
  }
 }
} 
