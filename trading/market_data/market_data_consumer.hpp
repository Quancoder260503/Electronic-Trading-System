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
    
  private:
    size_t next_expected_sequence_number = 1; 
    Exchange::MarketUpdateLFQueue incoming_md_queue = nullptr; 
    volatile bool is_running = false; 
    Common::McastSocket incremental_mcast_socket, snapshot_mcast_socket; 
    bool in_recovery_mode = false; 
    const std::string iface, snapshot_ip; 
    const int snapshot_port; 
    typedef std::map<size_t, Exchange::MatchingEngineMarketUpdate>QueuedMarketUpdates; 
    QueuedMarketUpdates snapshot_queued_msgs, incremental_queued_msgs;
    
    auto run() noexcept -> void; 

    auto recvCallback(McastSocket *socket) noexcept -> void; 

    auto queueMessage(bool is_snapshot, const Exchange::MDPMatchingEngineMarketUpdate *updates); 

    auto startSnapshotSync() -> void; 

    auto checkSnapshotSync() -> void; 

 }
}