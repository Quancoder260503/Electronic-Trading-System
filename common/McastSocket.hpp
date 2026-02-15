#pragma once 

#include <functional>
#include "SocketUtil.hpp"
#include "Logging.hpp"

namespace Common { 
 constexpr size_t MULTICAST_BUFFER_SIZE = 64 * 1024 * 1024;
 
 struct McastSocket { 
  McastSocket(Logger &logger_) : logger(logger_) { 
   recv_buffer.resize(MULTICAST_BUFFER_SIZE); 
   send_buffer.resize(MULTICAST_BUFFER_SIZE); 
  }

  auto init(const std::string &ip, const std::string &iface, int port, bool listening) -> int; 

  auto join(const std::string &ip) -> bool; 

  auto leave(const std::string &ip, int port) -> void; 

  auto sendAndRecv() noexcept -> bool; 

  auto send(const void *data, size_t len) noexcept -> void; 

  int socket_fd = -1; 
  
  size_t next_send_valid_index = 0; 
  size_t next_recv_valid_index = 0; 
  std::vector<char> recv_buffer; 
  std::vector<char> send_buffer; 

  std::function<void(McastSocket *s)> recv_callback = nullptr; 

  std::string time_str; 
  Logger &logger; 
 }; 
}