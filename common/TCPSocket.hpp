#pragma once
#include <functional>

#include "Logging.hpp"
#include "common/SocketUtil.hpp"

namespace Common {
constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

struct TCPSocket {
  explicit TCPSocket(Logger& logger_) : logger(logger_) {
    send_buffer.resize(TCPBufferSize);
    recv_buffer.resize(TCPBufferSize);
  }

  // Create TCPSocket with provided attributes to either listen-on or connect-to
  auto connect(const std::string& ip, const std::string& iface, int port,
               bool is_listening) noexcept -> int;

  // Called to write send data from buffers as well as check the data from the
  // recv buffer
  auto sendAndRecv() noexcept -> bool;

  // Write data in the send buffer
  auto send(const void* data, size_t len) noexcept -> void;

  TCPSocket() = delete;

  TCPSocket(const TCPSocket&) = delete;

  TCPSocket(const TCPSocket&&) = delete;

  TCPSocket& operator=(const TCPSocket&) = delete;

  TCPSocket& operator=(const TCPSocket&&) = delete;

  int socket_fd = -1;

  std::vector<char> send_buffer;
  size_t next_send_valid_index = 0;
  std::vector<char> recv_buffer;
  size_t next_recv_valid_index = 0;

  struct sockaddr_in socket_attribute{};

  // Function Wrapper to callback when there is data to be processed.
  std::function<void(TCPSocket* s, Nanos rx_time)> recv_callback = nullptr;

  std::string time_str;
  Logger& logger;
};
}  // namespace Common