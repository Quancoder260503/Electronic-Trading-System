#include "Logging.hpp"
#include "TCPServer.hpp"
#include "TCPSocket.hpp"

using namespace Common;

int main(void) {
  std::string time_str;
  Logger logger("socket_example.log");
  auto tcpServerRecvCallback = [&](TCPSocket* socket,
                                   Nanos rx_time) noexcept -> void {
    logger.log("TCPServer::defaultRecvCallback() socket:% len:% rx:%\n",
               socket->socket_fd, socket->next_recv_valid_index, rx_time);

    const std::string reply =
        "TCPServer received msg : " +
        std::string(socket->recv_buffer.data(), socket->next_recv_valid_index);
    socket->next_recv_valid_index = 0;
    socket->send(reply.data(), reply.length());
  };

  auto tcpServerRecvFinishedCallback = [&]() noexcept -> void {
    logger.log("TCPServer::defaultRecvFinishedCallback()\n");
  };

  auto tcpClientRecvCallback = [&](TCPSocket* socket,
                                   Nanos rx_time) noexcept -> void {
    const std::string recv_msg =
        std::string(socket->recv_buffer.data(), socket->next_recv_valid_index);
    socket->next_recv_valid_index = 0;
    logger.log("TCPSocket::defaultRecvCallback() socket:% len:% rx:% msg:%\n",
               socket->socket_fd, socket->next_recv_valid_index, rx_time,
               recv_msg);
  };

  const std::string iface = "lo";
  const std::string ip = "127.0.0.1";
  const int port = 8001;

  logger.log("Creating TCPServer on iface:% port:%\n", iface, port);
  TCPServer server(logger);
  server.recv_callback = tcpServerRecvCallback;
  server.recv_finished_callback = tcpServerRecvFinishedCallback;
  server.listen(iface, port);

  std::vector<TCPSocket*> clients(5);
  for (size_t i = 0; i < clients.size(); i++) {
    clients[i] = new TCPSocket(logger);
    clients[i]->recv_callback = tcpClientRecvCallback;
    logger.log("Connecting TCPClient-[%] on ip:% iface:% port:%\n", i, ip,
               iface, port);
    clients[i]->connect(ip, iface, port, false);
    server.poll();
  }

  using namespace std::literals::chrono_literals;

  for (auto itr = 0; itr < 5; itr++) {
    for (size_t i = 0; i < clients.size(); i++) {
      const std::string client_msg = "CLIENT-[" + std::to_string(i) +
                                     "] : Sending " +
                                     std::to_string(itr * 100 + i);
      logger.log("Sending TCPClient-[%] %\n", i, client_msg);
      clients[i]->send(client_msg.data(), client_msg.length());
      clients[i]->sendAndRecv();
      // std::this_thread::sleep_for(500ms);
      server.poll();
      server.sendAndRecv();
    }
  }

  return 0;
}