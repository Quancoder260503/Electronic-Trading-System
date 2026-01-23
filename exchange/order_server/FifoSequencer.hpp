#pragma once 
#include "common/ThreadUtil.hpp"
#include "common/Macros.hpp"
#include "common/TimeUtil.hpp"
#include "ClientRequest.hpp"

namespace Exchange { 
  constexpr size_t MATCHING_ENGINE_MAX_PENDING_REQUESTS = 1024; 
  class FIFOSequencer { 
   public : 
    FIFOSequencer(ClientRequestLFQueue *client_requests, Logger *logger_) : 
        incoming_requests(client_requests), logger(logger_) { }
    ~FIFOSequencer(){

    }
    auto addClientRequest(Nanos rx_time, const MatchingEngineClientRequest &request) {
     if(pending_size >= pending_client_requests.size()) { 
      FATAL("Too many requests"); 
     }   
     pending_client_requests.at(pending_size++) = std::move(RecvTimeClientRequest{rx_time, request}); 
    } 

    auto sequenceAndPublish() { 
     if(UNLIKELY(!pending_size)) return; 
      logger_->log("%:% %() % Processing % requests.\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str), pending_size); 
      std::sort(pending_client_requests.begin(), pending_client_requests.begin() + pending_size); 
      
      for(size_t i = 0; i < pending_size; i++) { 
        const auto &client_request = pending_client_requests.at(i); 
        logger->log("%:% %() % Writing RX:% Req:% to FIFO.\n", 
            __FILE__, __LINE__, __FUNCTION__, 
            Common::getCurrentTimeStr(&time_str),
            client_request.recv_time, 
            client_request.request.toString()
        );
        auto next_write = incoming_requests->getNextToWrite(); 
        (*next_write) = std::move(client_request.request); 
        incoming_requests->updateWriteIndex(); 
      }
      pending_size = 0; 
    }

    FIFOSequencer() = delete; 
    FIFOSequencer(const FIFOSequencer &) = delete; 
    FIFOSequencer(const FIFOSequencer &&) = delete; 
    FIFOSequencer &operator = (const FIFOSequencer &) = delete; 
    FIFOSequencer &operator = (const FIFOSequencer &&) = delete; 

   private:
    ClientRequestLFQueue *incoming_requests = nullptr; 
    std::string time_str; 
    Logger *logger = nullptr; 
    struct RecvTimeClientRequest { 
     Nanos recv_time = 0; 
     MatchingEngineClientRequest request; 
     auto operator < (const RecvTimeClientRequest &rhs) const { 
      return recv_time < rhs.recv_time; 
     }
    };
    std::array<RecvTimeClientRequest, MATCHING_ENGINE_MAX_PENDING_REQUESTS> pending_client_requests; 
    size_t pending_size = 0; 
  }
}