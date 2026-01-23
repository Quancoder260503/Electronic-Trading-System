#pragma once 

#include "common/Types.hpp"
#include "common/ThreadUtil.hpp"
#include "common/LockFreeQueue.hpp"
#include "common/Macros.hpp"
#include "common/McastSocket.hpp"
#include "common/Mempool.hpp"
#include "common/Logging.hpp"
#include "MarketUpdate.hpp" 
#include "matching/Order.hpp"

using namespace Common; 

namespace Exchange { 
   class SnapshotSynthesizer { 
    public: 
      SnapshotSynthesizer(MDPMarketUpdaLFQueue *market_updates, const std::string &iface, 
         const std::string &snapshot_ip, int snapshot_port); 
             
      ~SnapshotSynthesizer (); 

      auto start() noexcept -> void; 

      auto stop() noexcept -> void; 

      auto run() noexcept -> void; 
    
      auto publishSnapshot(); 

      auto addToSnapshot(const MDPMarketUpdate *market_update); 

      SnapshotSynthesizer () = delete; 
      SnapshotSynthesizer(const SnapshotSynthesizer &)  = delete; 
      SnapshotSynthesizer(const SnapshotSynthesizer &&) = delete;
      SnapshotSynthesizer &operator = (const SnapshotSynthesizer &)  = delete; 
      SnapshotSynthesizer &operator = (const SnapshotSynthesizer &&) = delete; 
    private : 
      // Lock free queue containing incremental market data updates  
      MDPMarketUpdateLFQueue *snapshot_md_updates = nullptr; 
      Logger logger;
      volatile bool is_running = false; 

      std::string time_str; 

      // Multicast socket for the snapshot multicast stream 
      McastSocket snapshot_socket; 
      
      // Hashmap that maps TickerID -> the order book snapshots 
      std::array<std::array<MatchingEngineMarketUpdate*, MATCHING_ENGINE_MAX_ORDER_IDS>, MATCHING_ENGINE_MAX_TICKERS> ticker_orders; 
      size_t last_increment_sequence_number = 0; 
      Nanos  last_snapshot_time = 0; 
      
      // Mempool used to manage update messages for the orders in the snapshot limit order books. 
      MemPool<MatchingEngineMarketUpdate> order_pool; 
   };
}