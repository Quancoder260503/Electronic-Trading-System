#pragma once 
#include <functional>
#include "SocketUtil.hpp"
#include "Logging.hpp"

namespace Common { 
   constexpr size_t TCPBufferSize = 64 * 1024 * 1024;
   struct TCPSocket { 
    
    int fd = -1; 
    char *send_buffer = nullptr; 
    size_t next_send_valid_index = 0; 
    char *rcv_buffer  = nullptr; 
    size_t next_recv_valid_index  = 0; 
    bool send_disconnected = false; 
    bool recv_disconnected = false; 
    struct sockaddr_in inInAddr; 
    std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback; 
    std::string time_str; 
    Logger &logger; 

    auto defaultRecvCallback(TCPSocket *socket, Nanos rx_time) noexcept { 
      logger_.log("%:% %() % TCPSocket::defaultRecvCallback() socket:% len:% rx:%\n", __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str_),
        socket->fd_, 
        socket->next_rcv_valid_index_, rx_time
      );
    }
    
    explicit TCPSocket(Logger &logger_) : logger(logger_) { 
      send_buffer = new char[TCPBufferSize]; 
      recv_buffer = new char[TCPBufferSize]; 
      recv_callback = [this](auto socket, auto rx_time) { 
        defaultRecvCallback(socket, rx_time);  
      };   
    }
    
    auto destroy() noexcept -> void { 
      close(fd); 
      fd = -1; 
    }

    ~TCPSocket() { 
     destroy(); 
     delete[] send_buffer; 
     send_buffer = nullptr; 
     delete[] recv_buffer; 
     recv_buffer = nullptr; 
    }

    TCPSocket() = delete; 
    TCPSocket(const TCPSocket &) = delete; 
    TCPSocket(const TCPSocket &&) = delete; 
    TCPSocket &operator = (const TCPSocket &) = delete; 
    TCPSocket &operator = (const TCPSocket &&) = delete; 

  }; 
}