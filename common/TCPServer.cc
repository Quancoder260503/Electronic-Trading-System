#include "common/TCPServer.hpp"

namespace Common {
// Start listening for connections on the provided interface and port
auto TCPServer::listen(const std::string& iface, int port) -> void {
  epoll_fd = epoll_create(1);
  ASSERT(epoll_fd >= 0,
         "epoll_create() failed error : " + std::string(std::strerror(errno)));

  ASSERT(listener_socket.connect("", iface, port, true) >= 0,
         "Listener socket failed to connect. interface : " + iface +
             " port : " + std::to_string(port) +
             " error : " + std::string(std::strerror(errno)));

  ASSERT(addToEpollList(&listener_socket) == EXIT_SUCCESS,
         "epoll_ctl() failed. error : " + std::string(std::strerror(errno)));
}

// Check for new connections or dead connections and update containers that
// track sockets
auto TCPServer::poll() noexcept -> void {
  const size_t max_events = 1 + send_sockets.size() + receive_sockets.size();
  const int n = epoll_wait(epoll_fd, events, static_cast<int>(max_events), 0);
  bool have_new_connection = false;
  for (int i = 0; i < n; i++) {
    const auto& event = events[i];
    auto socket = reinterpret_cast<TCPSocket*>(event.data.ptr);
    // have new connections
    if (event.events & EPOLLIN) {
      if (socket == &listener_socket) {
        logger.log("%:% %() % EPOLLIN listener_socket:%\n", __FILE__, __LINE__,
                   __FUNCTION__, Common::getCurrentTimeStr(&time_str),
                   socket->socket_fd);
        have_new_connection = true;
        continue;
      }
      logger.log("%:% %() % EPOLLIN socket:%\n", __FILE__, __LINE__,
                 __FUNCTION__, Common::getCurrentTimeStr(&time_str),
                 socket->socket_fd);
      if (std::find(receive_sockets.begin(), receive_sockets.end(), socket) ==
          receive_sockets.end()) {
        receive_sockets.push_back(socket);
      }
    }
    if (event.events & EPOLLOUT) {
      logger.log("%:% %() % EPOLLOUT socket:%\n", __FILE__, __LINE__,
                 __FUNCTION__, Common::getCurrentTimeStr(&time_str),
                 socket->socket_fd);
      if (std::find(send_sockets.begin(), send_sockets.end(), socket) ==
          send_sockets.end()) {
        send_sockets.push_back(socket);
      }
    }
    if (event.events & (EPOLLHUP | EPOLLERR)) {
      logger.log("%:% %() % EPOLLERR socket:%\n", __FILE__, __LINE__,
                 __FUNCTION__, Common::getCurrentTimeStr(&time_str),
                 socket->socket_fd);
      if (std::find(receive_sockets.begin(), receive_sockets.end(), socket) ==
          receive_sockets.end()) {
        receive_sockets.push_back(socket);
      }
    }
  }
  while (have_new_connection) {
    logger.log("%:% %() % have_new_connection\n", __FILE__, __LINE__,
               __FUNCTION__, Common::getCurrentTimeStr(&time_str));
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    int fd = accept(listener_socket.socket_fd,
                    reinterpret_cast<sockaddr*>(&addr), &addr_len);
    if (fd == -1) { break; }
    ASSERT(setNonBlockingFD(fd) && setNoDelay(fd),
           "Failed to set-non blocking or no-delay on socket : " +
               std::to_string(fd));

    logger.log("%:% %() % accepted socket:%\n", __FILE__, __LINE__,
               __FUNCTION__, Common::getCurrentTimeStr(&time_str), fd);

    auto socket = new TCPSocket(logger);
    socket->socket_fd = fd;
    socket->recv_callback = recv_callback;
    ASSERT(
        addToEpollList(socket) == EXIT_SUCCESS,
        "Unable to add socket. error : " + std::string(std::strerror(errno)));
    if (std::find(receive_sockets.begin(), receive_sockets.end(), socket) ==
        receive_sockets.end()) {
      receive_sockets.push_back(socket);
    }
  }
}

// Publish outgoing data from the send buffer and read incoming data from
// receive buffer
auto TCPServer::sendAndRecv() noexcept -> void {
  auto recv = false;
  std::for_each(receive_sockets.begin(), receive_sockets.end(),
                [&recv](auto socket) { recv |= socket->sendAndRecv(); });
  if (recv) { recv_finished_callback(); }
  std::for_each(receive_sockets.begin(), receive_sockets.end(),
                [](auto socket) { socket->sendAndRecv(); });
}

auto TCPServer::addToEpollList(TCPSocket* socket) noexcept -> bool {
  epoll_event event{EPOLLET | EPOLLIN, {reinterpret_cast<void*>(socket)}};
  return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket->socket_fd, &event);
}

}  // namespace Common