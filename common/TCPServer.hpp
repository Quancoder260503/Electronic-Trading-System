#pragma once

#include "common/TCPSocket.hpp"

namespace Common {
struct TCPServer {
  explicit TCPServer(Logger &logger_) : listener_socket(logger_), logger(logger_) {}

  // Start listening for connections on the provided interface and port
  auto listen(const std::string &iface, int port) -> void;

  // Check for new connections or dead connections and update containers that track sockets
  auto poll() noexcept -> void;

  // Publish outgoing data from the send buffer and read incoming data from receive buffer
  auto sendAndRecv() noexcept -> void;

 private:
  // Add socket to container
  auto addToEpollList(TCPSocket *socket) noexcept -> bool;

 public:
  int epoll_fd = -1;
  TCPSocket listener_socket;
  epoll_event events[(1 << 10)];

  // Collectionm of all sockets, sockets for incoming data, sockets for outgoing data and dead
  // connections
  std::vector<TCPSocket *> receive_sockets, send_sockets;

  // Function Wrapper to call back when data is available
  std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback = nullptr;

  // Function Wrapper to call back when all data across all TCPSockets
  std::function<void()> recv_finished_callback = nullptr;

  std::string time_str;
  Logger &logger;
};

}  // namespace Common