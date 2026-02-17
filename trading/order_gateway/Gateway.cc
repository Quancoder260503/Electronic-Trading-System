#include "Gateway.hpp"

namespace Trading { 
 OrderGateway::OrderGateway(ClientID client_id_, 
    Exchange::ClientRequestLFQueue *outgoing_request_, 
    Exchange::ClientResponseLFQueue *incoming_response_, 
    const std::string &ip_, 
    const std::string &iface_, 
    int port_) :
    client_id(client_id_), 
    outgoing_request(outgoing_request_), 
    incoming_response(incoming_response_), 
    ip(ip_), 
    iface(iface_), 
    port(port_), 
    logger("trading_order_gateway" + std::to_string(client_id) + ".log"), 
    tcp_socket(logger) {
      tcp_socket.recv_callback = [this](auto socket, auto rx_time) { 
        recvCallback(socket, rx_time); 
      }; 
 }

 OrderGateway::~OrderGateway() { 
  stop(); 

  using namespace std::literals::chrono_literals; 
  std::this_thread::sleep_for(5s); 
 }

 auto OrderGateway::start() noexcept -> void { 
  is_running = true; 
  ASSERT(tcp_socket.connect(ip, iface, port, false) >= 0, 
   "Unable to connect to ip: " + ip + " port :" + std::to_string(port) + " on iface : " + iface + " error: " + std::string(std::strerror(errno))
  ); 
  ASSERT(Common::createAndStartThread(-1, "Trading/OrderGateway", 
    [this]() { run(); }) != nullptr, "Failed to start order gateway thread."
  ); 
 }

 auto OrderGateway::stop() noexcept -> void { 
  is_running = false; 
 }

 auto OrderGateway::run() noexcept -> void { 
   logger.log(
    "%:% %() %\n", 
    __FILE__, __LINE__, __FUNCTION__, 
    Common::getCurrentTimeStr(&time_str)
   );
   while(is_running) { 
    tcp_socket.sendAndRecv(); 
    for(auto client_request = outgoing_request->getNextToRead(); 
      client_request != nullptr && outgoing_request->size(); 
      client_request = outgoing_request->getNextToRead()) { 

      logger.log("%:% %() % Sending cid:% seq:% %\n", 
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str), 
        client_id, 
        next_outgoing_sequence_number, 
        client_request->toString()
      );
      tcp_socket.send(&next_outgoing_sequence_number, sizeof(next_outgoing_sequence_number)); 
      tcp_socket.send(client_request, sizeof(Exchange::MatchingEngineClientRequest)); 
      outgoing_request->updateReadIndex(); 
      next_outgoing_sequence_number++; 
    }
   }
 }

 auto OrderGateway::recvCallback(Common::TCPSocket *socket, Common::Nanos rx_time) noexcept -> void { 
    logger.log("%:% %() % Received socket:% len:% %\n", 
     __FILE__, __LINE__, __FUNCTION__, 
     Common::getCurrentTimeStr(&time_str), 
     socket->socket_fd, 
     socket->next_recv_valid_index,
     rx_time
    ); 
    if(socket->next_recv_valid_index >= sizeof(Exchange::OrderManagementClientResponse)) { 
      size_t index = 0; 
      for(; index + sizeof(Exchange::OrderManagementClientResponse) <= socket->next_recv_valid_index; 
            index += sizeof(Exchange::OrderManagementClientResponse)) { 
        auto response = reinterpret_cast<Exchange::OrderManagementClientResponse*>(
         socket->recv_buffer.data() + index    
        ); 
        logger.log("%:% %() % Received %\n", 
         __FILE__, __LINE__, __FUNCTION__, 
         Common::getCurrentTimeStr(&time_str), 
         response->toString()
        );
        if(response->me_client_response.client_id != client_id) { // This should never happens
          logger.log("%:% %() % ERROR Incorrect client id. ClientId expected:% received:%.\n", 
                    __FILE__, __LINE__, __FUNCTION__,
                    Common::getCurrentTimeStr(&time_str), 
                    client_id, 
                    response->me_client_response.client_id
          );
          continue;  
        }
        if(response->sequence_number != next_expected_sequence_number) { 
          logger.log("%:% %() % ERROR Incorrect sequence number. ClientId:%. SeqNum expected:% received:%.\n", 
            __FILE__, __LINE__, __FUNCTION__,
            Common::getCurrentTimeStr(&time_str), 
            client_id, 
            next_expected_sequence_number, 
            response->sequence_number
          );
          continue;   
        }
        ++next_expected_sequence_number; 
        auto next_write = incoming_response->getNextToWrite();
        *next_write = response->me_client_response; 
        incoming_response->updateWriteIndex();  
      }
      memcpy(socket->recv_buffer.data(), socket->recv_buffer.data() + index, socket->next_recv_valid_index - index); 
      socket->next_recv_valid_index -= index; 
    }
 }
}