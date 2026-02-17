#include "OrderServer.hpp"

namespace Exchange { 
    OrderServer::OrderServer(ClientRequestLFQueue *client_request, 
    ClientResponseLFQueue *client_response, 
    const std::string &iface_, int port_) :

    iface(iface_), port(port_), 
    outgoing_responses(client_response),
    logger("exchange_order_server.log"), 
    tcp_server(logger), 
    fifo_sequencer(client_request, &logger) {
      cid_next_expected_sequence_number.fill(1); 
      cid_next_outgoing_sequence_number.fill(1); 
      cid_tcp_sockets.fill(nullptr); 
      tcp_server.recv_callback = [this](auto socket, auto rx_time) { 
        recvCallBack(socket, rx_time); 
      }; 
      tcp_server.recv_finished_callback = [this]() { 
        recvFinishedCallBack(); 
      }; 
    }
    
    OrderServer::~OrderServer() { 
     stop(); 
     using namespace std::literals::chrono_literals; 
     std::this_thread::sleep_for(1s);
    }

    auto OrderServer::start() -> void { 
     is_running = true; 
     tcp_server.listen(iface, port); 
     ASSERT(Common::createAndStartThread(-1, "exchange/order_server", [this]() { 
      run(); 
     }) != nullptr, "Failed to start OrderServer thread."); 
    }

    auto OrderServer::stop() -> void { 
      is_running = false;
    }
}