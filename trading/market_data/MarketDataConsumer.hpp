#pragma once 

#include <functional> 
#include <map> 

#include "common/ThreadUtil.hpp"
#include "common/LockFreeQueue.hpp"
#include "common/Macros.hpp"
#include "common/McastSocket.hpp"
#include "exchange/market_data/MarketUpdate.hpp"

namespace Trading { 
 class MarketDataConsumer { 
  public: 
    MarketDataConsumer(Common::ClientID client_id, Exchange::MarketUpdateLFQueue *market_update_queue, const std::string &iface_, 
                       const std::string &snapshot_ip_, int snapshot_port_, 
                       const std::string &incremental_ip_, int incremental_port_); 
    
    ~MarketDataConsumer(); 
    
    auto start() noexcept -> void; 
    auto stop()  noexcept -> void; 


    MarketDataConsumer() = delete; 
    MarketDataConsumer(const MarketDataConsumer&) = delete; 
    MarketDataConsumer(const MarketDataConsumer&&) = delete; 
    MarketDataConsumer &operator = (const MarketDataConsumer &) = delete; 
    MarketDataConsumer &operator = (const MarketDataConsumer &&) = delete; 
    
  private:
    // Track the sequence number on the incremental market data stream, used to detect drop-off packets
    size_t next_expected_sequence_number = 1; 

    Exchange::MarketUpdateLFQueue *incoming_md_queue = nullptr; 
    
    volatile bool is_running = false; 
   
    std::string time_str; 
    Logger logger; 

    // Multicast subscriber socket for the incremental and market data streams 
    Common::McastSocket incremental_mcast_socket, snapshot_mcast_socket; 
    
    // Tracks if we are currently in the process of recovering / synchronizing with the snapshot of market data stream 
    bool in_recovery_mode = false;

    // Information for the snapshot multicast stream 
    const std::string iface, snapshot_ip, incremental_ip; 
    const int snapshot_port, incremental_port;
     
    // Containers to queue up market data updates from the snapshot and incremental channels in order of increasing sequence numbers 
    typedef std::map<size_t, Exchange::MatchingEngineMarketUpdate>QueuedMarketUpdates; 
    QueuedMarketUpdates snapshot_queued_msgs, incremental_queued_msgs;
    

    // Main loop for this thread -> reads and processes incoming messages from multicast
    auto run() noexcept -> void; 

    // Process a market data update, consumer needs to use the socket parameter to find which stream the data came from
    auto recvCallback(McastSocket *socket) noexcept -> void; 

    // Queued up the messages 
    auto queueMessage(bool is_snapshot, const Exchange::MDPMarketUpdate *updates); 

    // Start the process of snapshot/synchronization by subscribing to the snapshot multicast stream 
    auto startSnapshotSync() noexcept -> void; 

    // Check if a recovery/synchronization is possible from the queued up market data updates from the snapshot and incremental stream 
    auto checkSnapshotSync() noexcept -> void; 
 }; 
}