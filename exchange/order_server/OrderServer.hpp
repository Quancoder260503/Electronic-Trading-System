#pragma once 

#include <functional>
#include <string> 
#include "common/ThreadUtil.hpp"
#include "common/Macros.hpp"
#include "common/TCPServer.hpp"
#include "ClientResponse.hpp"
#include "ClientRequest.hpp"
#include "FifoSequencer.hpp"

namespace Exchange {
 class OrderServer { 
  public:
  
   OrderServer(ClientRequestLFQueue *client_requests, ClientResponseLFQueue *client_responses, const std::string &iface, int port); 
   ~OrderServer(); 

   auto start() -> void; 
   auto stop() -> void; 
  
   auto run() noexcept { 
    logger.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
    while(is_running) { 
      tcp_server.poll(); 
      tcp_server.sendAndRecv(); 

      for(auto client_response = outgoing_responses->getNextToRead(); 
          outgoing_responses->size() && client_response; 
          client_response = outgoing_responses->getNextToRead()) { 
        
        auto &next_outgoing_sequence_number = cid_next_outgoing_sequence_number.at(client_response->client_id); 
        logger.log(
          "%:% %() % Processing cid:% seq:% %\n",
           __FILE__, __LINE__, __FUNCTION__, 
           Common::getCurrentTimeStr(&time_str),
           client_response->client_id, 
           next_outgoing_sequence_number, 
           client_response->toString()
        );
        ASSERT(cid_tcp_sockets[client_response->client_id] != nullptr, 
          "Do not have a TCPSocket for ClientID : " + std::to_string(client_response->client_id));
        
        cid_tcp_sockets[client_response->client_id]->send(&next_outgoing_sequence_number, sizeof(next_outgoing_sequence_number));
        cid_tcp_sockets[client_response->client_id]->send(client_response, sizeof(MatchingEngineClientResponse)); 
        outgoing_responses->updateReadIndex(); 
        ++next_outgoing_sequence_number;
      }
    }
   }

   // Receive data from clients and put into the FIFO sequencer 
   auto recvCallBack(TCPSocket *socket, Nanos rx_time) noexcept { 
    logger.log("%:% %() % Received socket:% len:% rx:%\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str),
        socket->socket_fd, socket->next_recv_valid_index, rx_time);
    if(socket->next_recv_valid_index >= sizeof(OrderManagementClientRequest)) { 
     size_t index = 0;
     for(; index + sizeof(OrderManagementClientRequest) <= socket->next_recv_valid_index; index += sizeof(OrderManagementClientRequest)) { 
       auto request = reinterpret_cast<const OrderManagementClientRequest*>(socket->recv_buffer.data() + index); 
       logger.log("%:% %() % Received %\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str), request->toString()
       );
       if(UNLIKELY(cid_tcp_sockets[request->me_client_request.client_id] == nullptr)) { 
        cid_tcp_sockets[request->me_client_request.client_id] = socket; 
       }
       if(cid_tcp_sockets[request->me_client_request.client_id] != socket) { 
        logger.log("%:% %() % Received ClientRequest from ClientId:% on different socket:% expected:%\n", __FILE__, __LINE__, __FUNCTION__,
                    Common::getCurrentTimeStr(&time_str), 
                    request->me_client_request.client_id, 
                    socket->socket_fd,
                    cid_tcp_sockets[request->me_client_request.client_id]->socket_fd);
        continue;
       }
       auto &next_expected_sequence_number = cid_next_expected_sequence_number[request->me_client_request.client_id];
       if(request->sequence_number != next_expected_sequence_number) { 
         logger.log("%:% %() % Incorrect sequence number. ClientId:% SeqNum expected:% received:%\n", __FILE__, __LINE__, __FUNCTION__,
                    Common::getCurrentTimeStr(&time_str), 
                    request->me_client_request.client_id, 
                    next_expected_sequence_number, 
                    request->sequence_number
         );
         continue;
       }
       ++next_expected_sequence_number; 
       fifo_sequencer.addClientRequest(rx_time, request->me_client_request);  
     }
     // clip the processed part
     memcpy(socket->recv_buffer.data(), socket->recv_buffer.data() + index, socket->next_recv_valid_index - index); 
     socket->next_recv_valid_index -= index; 
    }
  }
  
  auto recvFinishedCallBack() noexcept { 
   fifo_sequencer.sequenceAndPublish(); 
  }

  OrderServer() = delete; 
  OrderServer(const OrderServer&) = delete; 
  OrderServer(const OrderServer&&) = delete; 
  OrderServer &operator = (const OrderServer &) = delete; 
  OrderServer &operator = (const OrderServer&&) = delete; 

  private: 
   const std::string iface; 
   const int port = 0; 
   // Lock Free Queue of outgoing client responses to be sent out to the connected client 
   ClientResponseLFQueue *outgoing_responses = nullptr; 
   volatile bool is_running = false; // shared among threads 
   std::string time_str; 
   Logger logger; 
   // Array that stores the outgoing sequence number corresponding to client id 
   std::array<size_t, MATCHING_ENGINE_MAX_NUM_CLIENTS> cid_next_outgoing_sequence_number;  
   // Array that stores the expected sequence number to be received from the client
   std::array<size_t, MATCHING_ENGINE_MAX_NUM_CLIENTS> cid_next_expected_sequence_number; 

   std::array<Common::TCPSocket*, MATCHING_ENGINE_MAX_NUM_CLIENTS>cid_tcp_sockets; 

   Common::TCPServer tcp_server; 
   FIFOSequencer fifo_sequencer; 
 };
}