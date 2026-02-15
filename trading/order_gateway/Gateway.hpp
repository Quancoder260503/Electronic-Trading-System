#pragma once 

#include <functional>
#include "common/ThreadUtil.hpp"
#include "common/LockFreeQueue.hpp"
#include "common/TCPServer.hpp"
#include "exchange/order_server/ClientRequest.hpp"
#include "exchange/order_server/ClientResponse.hpp"

namespace Trading { 
 class OrderGateway { 
  public: 
    OrderGateway(ClientID client_id_, 
                 Exchange::ClientRequestLFQueue *outgoing_request_, 
                 Exchange::ClientResponseLFQueue *incoming_response_, 
                 const std::string &ip_, 
                 const std::string &iface_, 
                 int port_);

    ~OrderGateway(); 
    
    auto start() noexcept -> void; 

    auto stop() noexcept ->void; 


    OrderGateway() = delete;
    OrderGateway(const OrderGateway &) = delete;
    OrderGateway(const OrderGateway &&) = delete;
    OrderGateway &operator=(const OrderGateway &) = delete;
    OrderGateway &operator=(const OrderGateway &&) = delete;

  private: 
    const ClientID client_id; 
    Exchange::ClientRequestLFQueue *outgoing_request = nullptr; 
    Exchange::ClientResponseLFQueue *incoming_response = nullptr;
    std::string ip;  
    const std::string iface; 
    const int port = 0; 
    
    volatile bool is_running = false; 
    Logger logger; 
    size_t next_outgoing_sequence_number = 1; 
    size_t next_expected_sequence_number = 1; 
    Common::TCPSocket tcp_socket; 

    std::string time_str; 

    auto run() noexcept-> void; 
    auto recvCallback(Common::TCPSocket *s, Common::Nanos rx_time) noexcept -> void; 
 }; 
}

