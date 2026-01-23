#include "McastSocket.hpp" 
#include "Macros.hpp"

namespace Common { 
  auto McastSocket::init(const std::string &ip, const std::string &iface, int port, bool is_listening) -> int { 
    const SocketCfg socket_cfg{ip, iface, port, true, is_listening, false};
    socket_fd = createSocket(logger, socket_cfg); 
    return socket_fd;  
  }  

  bool McastSocket::join(const std::string &ip) { 
    return Common::join(socket_fd, ip); 
  }
  
  bool McastSocket::leave(const std::string &, int) -> void { 
    close(socket_fd); 
    socket_fd = -1; 
  } 

  auto McastSocket::sendAndRecv() noexcept -> bool { 
    const ssize_t n_recv = recv(socket_fd, recv_buffer.data() + next_recv_valid_index, MULTICAST_BUFFER_SIZE - next_recv_valid_index); 
    if(n_recv > 0) { 
      next_recv_valid_index += n_recv; 
      logger.log("%:% %() % read socket:% len:%\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str), socket_fd, next_recv_valid_index);
      recv_callback(this); 
    }   
    if(next_send_valid_index > 0) { 
      ssize_t n_send = ::send(socket_fd, send_buffer.data(), next_send_valid_index, MSG_DONTWAIT | MSGNOSIGNAL); 
      logger.log("%:% %() % send socket:% len:%\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str), socket_fd, n_send);
      next_send_valid_index = 0;    
    }
    return (n_recv > 0); 
  }

  auto McastSocket::send(const void *data, size_t len) noexcept -> void { 
   memcpy(send_buffer.data() + next_send_valid_index, data, len); 
   next_send_valid_index += len; 
   ASSERT(next_send_valid_index < MULTICAST_BUFFER_SIZE, 
      "Mcast socket buffer filled up and sendAndRecv() not called. "); 
  }
}