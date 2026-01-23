#pragma once 

#include <functional> 
#include "MarketUpdate.hpp"
#include "common/McastSocket.hpp"

namespace Exchange { 
 class MarketDataPublisher {
   public : 
    MarketDataPublisher(MarketUpdateLFQueue *market_updates, const std::string &iface, 
            const std::string &snapshot_ip, int snapshot_port, 
            const std::string &incremental_ip, int incremental_port);
             
    ~MarketDataPublisher(); 

    auto start() noexcept -> void; 

    auto stop() noexcept -> void; 

    auto run() noexcept -> void; 
    
    MarketDataPublisher() = delete; 
    MarketDataPublisher(const MarketDataPublisher&)  = delete; 
    MarketDataPublisher(const MarketDataPublisher&&) = delete;
    MarketDataPublisher &operator = (const MarketDataPublisher&)  = delete; 
    MarketDataPublisher &operator = (const MarketDataPublisher&&) = delete; 
    
   private : 
    // Sequence number to keep track on the incremental stream 
    size_t next_increment_sequence_number = 1; 

    // Lock free queue from which we consume the update sent by matching engines 
    MarketUpdateLFQueue *outgoing_market_updates = nullptr; 
    
    // Lock free queue on which we forward the incremental market data updates sent to the snapshot synthesis
    MDPMarketUpdateLFQueue snapshot_market_updates; 
    
    volatile bool is_running = false; 

    std::string time_str; 
    Logger logger;
    // Multicast socket to propagate incremental data stream    
    MCastSocket incremental_socket; 
    // Snapshot synthesize which synthesizes and publishes limit order book snapshots 
    SnapshotSynthesizer *snapshot_synthesizer = nullptr; 
 }; 
}