#include "common/TCPSocket.hpp"

namespace Common {

// Create TCPSocket with provided attributes to either listen-on or connect-to
auto TCPSocket::connect(const std::string& ip, const std::string& iface,
                        int port, bool is_listening) noexcept -> int {
  const SocketConfig socket_config{ip, iface, port, false, is_listening, true};
  socket_fd = createSocket(logger, socket_config);
  socket_attribute.sin_addr.s_addr = INADDR_ANY;
  socket_attribute.sin_port = htons(static_cast<__uint16_t>(port));
  socket_attribute.sin_family = AF_INET;
  return socket_fd;
}

// Called to write send data from buffers as well as check the data from the
// recv buffer
auto TCPSocket::sendAndRecv() noexcept -> bool {
  char ctrl[CMSG_SPACE(sizeof(struct timeval))];
  auto cmsg = reinterpret_cast<struct cmsghdr*>(&ctrl);

  iovec iov{recv_buffer.data() + next_recv_valid_index,
            TCPBufferSize - next_recv_valid_index};
  msghdr msg{&socket_attribute,
             sizeof(socket_attribute),
             &iov,
             1,
             ctrl,
             sizeof(ctrl),
             0};

  const auto read_size = recvmsg(socket_fd, &msg, MSG_DONTWAIT);
  if (read_size > 0) {
    next_recv_valid_index += read_size;
    Nanos kernel_time = 0;
    timeval time_kernel;
    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP &&
        cmsg->cmsg_len == CMSG_LEN(sizeof(time_kernel))) {
      memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));
      kernel_time = time_kernel.tv_sec * NANOS_TO_SECS +
                    time_kernel.tv_usec *
                        NANOS_TO_MICROS;  // convert timestamp to nanoseconds
    }
    const auto user_time = getCurrentNanos();
    logger.log("%:% %() % read socket:% len:% utime:% ktime:% diff:%\n",
               __FILE__, __LINE__, __FUNCTION__,
               Common::getCurrentTimeStr(&time_str), socket_fd,
               next_recv_valid_index, user_time, kernel_time,
               (user_time - kernel_time));
    recv_callback(this, kernel_time);
  }
  if (next_send_valid_index > 0) {
    // This send is the POSIX function not of the class
    const auto n = ::send(socket_fd, send_buffer.data(), next_send_valid_index,
                          MSG_DONTWAIT | MSG_NOSIGNAL);
    logger.log("%:% %() % send socket:% len:%\n", __FILE__, __LINE__,
               __FUNCTION__, Common::getCurrentTimeStr(&time_str), socket_fd,
               n);
  }
  next_send_valid_index = 0;
  return (read_size > 0);
}

// Write data in the send buffer
auto TCPSocket::send(const void* data, size_t len) noexcept -> void {
  memcpy(send_buffer.data() + next_send_valid_index, data, len);
  next_send_valid_index += len;
}
}  // namespace Common