#include "MarketDataConsumer.hpp"

namespace Trading { 
 MarketDataConsumer::MarketDataConsumer(
    Common::ClientID client_id, Exchange::MarketUpdateLFQueue *market_update_queue, const std::string &iface_, 
    const std::string &snapshot_ip_, int snapshot_port_, 
    const std::string &incremental_ip_, int incremental_port_
 ) : incoming_md_queue(market_update_queue), 
      is_running(false), 
      logger("trading_market_data_consumer_" + std::to_string(client_id) + ".log"), 
      incremental_mcast_socket(logger), 
      snapshot_mcast_socket(logger), 
      iface(iface_), 
      snapshot_ip(snapshot_ip_), 
      snapshot_port(snapshot_port_), 
      incremental_ip(incremental_ip_), 
      incremental_port(incremental_port_) { 

      auto recv_callback = [this](auto socket) { 
        recvCallback(socket); 
      }; 
      incremental_mcast_socket.recv_callback = recv_callback;
      ASSERT(incremental_mcast_socket.init(incremental_ip, iface, incremental_port, true) >= 0, 
        "Unable to create incremental market data multicast socket"); 
      ASSERT(incremental_mcast_socket.join(incremental_ip),  
       "Join failed on:" + std::to_string(incremental_mcast_socket.socket_fd) + " error:" + std::string(std::strerror(errno))); 
      snapshot_mcast_socket.recv_callback = recv_callback;  
 }

 MarketDataConsumer::~MarketDataConsumer() { 
  stop(); 
  using namespace std::literals::chrono_literals; 
  std::this_thread::sleep_for(5s); 
 }

 auto MarketDataConsumer::start() noexcept -> void { 
  is_running = true; 
  ASSERT(Common::createAndStartThread(-1, "Trading/MarketDataConsumer", [this]() { 
    run(); }) != nullptr, "Failed to start MarketDataConsumer thread");   
 }

 auto MarketDataConsumer::stop() noexcept -> void  { 
  is_running = false; 
 }

 // Main loop for this thread -> reads and process incoming messages from multicast sockets
 auto MarketDataConsumer::run() noexcept -> void { 
  logger.log("%:% %() %\n", 
    __FILE__, __LINE__, __FUNCTION__, 
    Common::getCurrentTimeStr(&time_str)
  );
  while(is_running) { 
    incremental_mcast_socket.sendAndRecv(); 
    snapshot_mcast_socket.sendAndRecv(); 
  }
 }
 
 auto MarketDataConsumer::startSnapshotSync() noexcept -> void { 
  snapshot_queued_msgs.clear(); 
  incremental_queued_msgs.clear(); 
  
  ASSERT(snapshot_mcast_socket.init(snapshot_ip, iface, snapshot_port, true) >= 0, 
   "Unable to create snapshot mcast socker. error : " + std::string(std::strerror(errno))
  );
  
  ASSERT(snapshot_mcast_socket.join(snapshot_ip), 
   "Join failed on:" + std::to_string(snapshot_mcast_socket.socket_fd) + " error:" + std::string(std::strerror(errno))
  );
 }

 auto MarketDataConsumer::checkSnapshotSync() noexcept -> void { 
  if(snapshot_queued_msgs.empty()) { 
    return; 
  }
  const auto &first_msg = snapshot_queued_msgs.begin()->second; 
  // First message type must be SNAPSHOT_START
  if(first_msg.type != Exchange::MarketUpdateType::SNAPSHOT_START) { 
   logger.log("%:% %() % Returning because have not seen a SNAPSHOT_START yet.\n",
    __FILE__, __LINE__, __FUNCTION__, 
    Common::getCurrentTimeStr(&time_str)
   );
   snapshot_queued_msgs.clear(); 
   return; 
  }
  std::vector<Exchange::MatchingEngineMarketUpdate> final_events;
  auto have_complete_snapshot = true; 
  int next_snapshot_sequence_number = 0; 
  for(const auto &snapshot_ptr : snapshot_queued_msgs) { 
    logger.log("%:% %() % % => %\n", 
      __FILE__, __LINE__, __FUNCTION__,
      Common::getCurrentTimeStr(&time_str), snapshot_ptr.first, snapshot_ptr.second.toString()
    );  
    if(snapshot_ptr.first != next_snapshot_sequence_number) { 
      have_complete_snapshot = false; 
      logger.log("%:% %() % Detected gap in snapshot stream expected:% found:% %.\n",
         __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str), 
        next_snapshot_sequence_number, 
        snapshot_ptr.first, 
        snapshot_ptr.second.toString()
      );
      break; 
    }
    if(snapshot_ptr.second.type != Exchange::MarketUpdateType::SNAPSHOT_START && 
       snapshot_ptr.second.type != Exchange::MarketUpdateType::SNAPSHOT_END) { 
      final_events.push_back(snapshot_ptr.second); 
    }
    ++next_snapshot_sequence_number; 
  } 
  if(!have_complete_snapshot) { 
    logger.log(
      "%:% %() % Returning because found gaps in snapshot stream.\n",
      __FILE__, __LINE__, __FUNCTION__, 
      Common::getCurrentTimeStr(&time_str)
    );
    snapshot_queued_msgs.clear(); 
    return; 
  }

  const auto &last_snapshot_msg = snapshot_queued_msgs.rbegin()->second; 
  if(last_snapshot_msg.type != Exchange::MarketUpdateType::SNAPSHOT_END) { 
    logger.log(
      "%:% %() % Returning because have not seen a SNAPSHOT_END yet.\n",
      __FILE__, __LINE__, __FUNCTION__, 
      Common::getCurrentTimeStr(&time_str)
    );
    return; 
  }

  auto have_complete_incrementals = true; 
  size_t num_incrementals = 0; 
  next_expected_sequence_number = last_snapshot_msg.order_id + 1; 
  for(auto inc_itr = incremental_queued_msgs.begin(); inc_itr != incremental_queued_msgs.end(); inc_itr++) { 
    logger.log(
      "%:% %() % Checking next_exp:% vs. seq:% %.\n", 
      __FILE__, __LINE__, __FUNCTION__,
      Common::getCurrentTimeStr(&time_str), 
      next_expected_sequence_number, 
      inc_itr->first,
      inc_itr->second.toString()
    );
    if(inc_itr->first < next_expected_sequence_number) { 
      continue;
    }
    if(inc_itr->first != next_expected_sequence_number) { 
      logger.log(
        "%:% %() % Detected gap in incremental stream expected:% found:% %.\n", 
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str), 
        next_expected_sequence_number, 
        inc_itr->first, 
        inc_itr->second.toString()
      ); 
      have_complete_incrementals = false; 
      break; 
    }
    logger.log(
      "%:% %() % % => %\n", 
      __FILE__, __LINE__, __FUNCTION__,
      Common::getCurrentTimeStr(&time_str), 
      inc_itr->first, 
      inc_itr->second.toString()
    ); 
    if(inc_itr->second.type != Exchange::MarketUpdateType::SNAPSHOT_START && 
       inc_itr->second.type != Exchange::MarketUpdateType::SNAPSHOT_END) { 
      final_events.push_back(inc_itr->second);
    }
    ++next_expected_sequence_number; 
    ++num_incrementals; 
  }

  if(!have_complete_incrementals) { 
    logger.log(
      "%:% %() % Returning because have gaps in queued incrementals.\n",
      __FILE__, __LINE__, __FUNCTION__, 
      Common::getCurrentTimeStr(&time_str)
    );
    snapshot_queued_msgs.clear(); 
    return; 
  }

  for(const auto &itr : final_events) { 
    auto next_write = incoming_md_queue->getNextToWrite(); 
    *next_write = itr; 
    incoming_md_queue->updateWriteIndex(); 
  }
  logger.log(
    "%:% %() % Recovered % snapshot and % incremental orders.\n", 
    __FILE__, __LINE__, __FUNCTION__,
    Common::getCurrentTimeStr(&time_str), 
    snapshot_queued_msgs.size() - 2, 
    num_incrementals
  );

  snapshot_queued_msgs.clear(); 
  incremental_queued_msgs.clear(); 
  in_recovery_mode = false; 
  snapshot_mcast_socket.leave(snapshot_ip, snapshot_port); 
  return; 
 }

 auto MarketDataConsumer::queueMessage(bool is_snapshot, const Exchange::MDPMarketUpdate *request) { 
  if(is_snapshot) { 
    if(snapshot_queued_msgs.find(request->sequence_number) != snapshot_queued_msgs.end()) { 
      logger.log(
        "%:% %() % Packet drops on snapshot socket. Received for a 2nd time:%\n", 
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str), 
        request->toString()
      );
      snapshot_queued_msgs.clear(); 
    }
    snapshot_queued_msgs[request->sequence_number] = request->me_market_update; 
  } else { 
    incremental_queued_msgs[request->sequence_number] = request->me_market_update; 
  }
  logger.log(
    "%:% %() % size snapshot:% incremental:% % => %\n", 
    __FILE__, __LINE__, __FUNCTION__,
    Common::getCurrentTimeStr(&time_str), 
    snapshot_queued_msgs.size(), 
    incremental_queued_msgs.size(), 
    request->sequence_number, 
    request->toString()
  ); 
  checkSnapshotSync(); 
 }


 auto MarketDataConsumer::recvCallback(McastSocket *socket) noexcept -> void { 
  const auto is_snapshot = (socket->socket_fd == snapshot_mcast_socket.socket_fd); 
  if(UNLIKELY(is_snapshot && !in_recovery_mode)) { 
    socket->next_recv_valid_index = 0; 
    logger.log(
      "%:% %() % WARN Not expecting snapshot messages.\n",
      __FILE__, __LINE__, __FUNCTION__, 
      Common::getCurrentTimeStr(&time_str)
    );
    return; 
  }
  if(socket->next_recv_valid_index >= sizeof(Exchange::MDPMarketUpdate)) { 
    size_t index = 0; 
    for(; index + sizeof(Exchange::MDPMarketUpdate) <= socket->next_recv_valid_index; index += sizeof(Exchange::MDPMarketUpdate)) { 
      auto request = reinterpret_cast<const Exchange::MDPMarketUpdate *>(socket->recv_buffer.data() + index); 
      logger.log(
        "%:% %() % Received % socket len:% %\n",
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str),
        (is_snapshot ? "snapshot" : "incremental"), 
        sizeof(Exchange::MDPMarketUpdate), 
        request->toString()
      ); 
      const bool already_in_recovery = in_recovery_mode; 
      in_recovery_mode = (already_in_recovery || request->sequence_number != next_expected_sequence_number); 
      if(UNLIKELY(in_recovery_mode)) { 
        // Just entered the recovery mode 
        if(UNLIKELY(!already_in_recovery)) { 
          logger.log(
            "%:% %() % Packet drops on % socket. SeqNum expected:% received:%\n", 
            __FILE__, __LINE__, __FUNCTION__,
            Common::getCurrentTimeStr(&time_str), 
            (is_snapshot ? "snapshot" : "incremental"), 
            next_expected_sequence_number, 
            request->sequence_number
          );
          startSnapshotSync(); 
        }
        // queue up the market data update message and see if recovery/synchronization can be completed successfully
        queueMessage(is_snapshot, request); 
      } else if(!is_snapshot) { // not in recovery mode and received a packet in correct order 
        logger.log(
          "%:% %() % %\n",
           __FILE__, __LINE__, __FUNCTION__,
          Common::getCurrentTimeStr(&time_str), 
          request->toString()
        );
        ++next_expected_sequence_number; 
        auto next_write = incoming_md_queue->getNextToWrite(); 
        *next_write = request->me_market_update; 
        incoming_md_queue->updateWriteIndex(); 
      }
    }
    memcpy(socket->recv_buffer.data(), socket->recv_buffer.data() + index, socket->next_recv_valid_index - index); 
    socket->next_recv_valid_index -= index; 
  }
  return; 
 }
}; 