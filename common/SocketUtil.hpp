#pragma once 
#include <iostream>
#include <sstream> 
#include <string> 
#include <unordered_set>
#include <sys/epoll.h> 
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h> 
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <ifaddrs.h> 
#include <fcntl.h> 
#include "common/Macros.hpp"
#include "common/Logging.hpp"

namespace Common { 

 struct SocketConfig { 
  std::string ip; 
  std::string iface; 
  int port = -1; 
  bool is_udp = false; 
  bool is_listening = false; 
  bool needs_so_timestamp = false; 

  auto toString() const {
    std::stringstream ss;
    ss << "SocketCfg[ip:" << ip      
       << " iface:" << iface
       << " port:" << port
       << " is_udp:" << is_udp
       << " is_listening:" << is_listening
       << " needs_SO_timestamp:" << needs_so_timestamp
       << "]";
      return ss.str();
  }
 }; 

 constexpr int MaxTCPServerBackLog = 1024;
 
 inline auto getIfaceIP(const std::string &iface) -> std::string { 
   char buffer[NI_MAXHOST] = {'\0'}; 
   ifaddrs *ifaddr = nullptr; 
   if(getifaddrs(&ifaddr) != -1) { 
    for(ifaddrs* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
       //std::cerr << "address : " << ifa->ifa_name << " " << ifa->ifa_addr->sa_family << '\n';  
      if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && iface == ifa->ifa_name)  {
        getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buffer, sizeof(buffer), NULL, 0, NI_NUMERICHOST); 
        break; 
      }  
    }
    freeifaddrs(ifaddr); 
   }
   return buffer; 
 }

 inline auto setNonBlockingFD(int fd) -> bool {  
    const auto flags = fcntl(fd, F_GETFL, 0); // Check if socket is already non-blocking 
    if(flags == -1) return false;             // Error 
    if(flags & O_NONBLOCK) return true;       // Already set 
    return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);  // Change the socket into non-block mode
 }

 inline auto setNoDelay(int fd) -> bool { 
   int one = 1; 
   return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, 
        reinterpret_cast<void *>(&one), sizeof(one)) != -1); 
 } 
 
 inline auto setSOTimestamp(int fd) -> bool { 
   int one = 1; 
   return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, 
    reinterpret_cast<void *>(&one), sizeof(one)) != -1); 
 } 

 inline auto wouldBlock() -> bool { 
  return (errno == EWOULDBLOCK || errno == EINPROGRESS); 
 }

 inline auto setMcastTTL(int fd, int mcast_ttl) -> bool {
   return (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, 
    reinterpret_cast<void *>(&mcast_ttl), sizeof(mcast_ttl)) != -1);   
 } 

 inline auto setTTL(int fd, int ttl) -> bool { 
   return (setsockopt(fd, IPPROTO_TCP, IP_TTL, 
    reinterpret_cast<void *>(&ttl), sizeof(ttl)) != -1); 
 }

 // add membership / subscription to the multicast stream specified
 inline auto join(int fd, const std::string &ip, const std::string &iface, int port) -> bool { 
  const ip_mreq mreq{ 
    {inet_addr(ip.c_str())}, 
    {htonl(INADDR_ANY)}
  }; 
  return (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1); 
 }
 
 // Create listening TCP/UDP socket 
 [[nodiscard]] inline auto createSocket(Logger& logger, const SocketConfig& socket_cfg) -> int {
    std::string time_str;
    const auto ip = socket_cfg.ip.empty() ? getIfaceIP(socket_cfg.iface) : socket_cfg.ip;
    logger.log("%:% %() % cfg:%\n",
        __FILE__, __LINE__, __FUNCTION__,
        Common::getCurrentTimeStr(&time_str),
        socket_cfg.toString());
    const int flags = (socket_cfg.is_listening ? AI_PASSIVE : 0) | AI_NUMERICHOST | AI_NUMERICSERV;
    addrinfo hints{};
    hints.ai_flags    = flags;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = socket_cfg.is_udp ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = socket_cfg.is_udp ? IPPROTO_UDP : IPPROTO_TCP;
    addrinfo* res = nullptr;
    const int rc = getaddrinfo(ip.c_str(), std::to_string(socket_cfg.port).c_str(), &hints, &res);
    ASSERT(rc == EXIT_SUCCESS, "getaddrinfo() failed: " + std::string(gai_strerror(rc)));
    int one = 1;
    for (addrinfo* rp = res; rp; rp = rp->ai_next) {
      int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (fd == -1) {
        logger.log("socket() failed: %\n", strerror(errno));
        continue;
      }
      if (!setNonBlockingFD(fd)) {
        logger.log("setNonBlockingFD() failed: %\n", strerror(errno));
        close(fd);
        continue;
      }
      if (!socket_cfg.is_udp && !setNoDelay(fd)) {
        logger.log("setNoDelay() failed: %\n", strerror(errno));
        close(fd);
        continue;
      }
      if (!socket_cfg.is_listening) {
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == -1 && errno != EINPROGRESS) {
          logger.log("connect() failed: %\n", strerror(errno));
          close(fd);
          continue;
        }
      } else {
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
          close(fd);
          continue;
        }
        if (bind(fd, rp->ai_addr, rp->ai_addrlen) == -1) {
          logger.log("bind() failed: %\n", strerror(errno));
          close(fd);
          continue;
        }
        if (!socket_cfg.is_udp) {
          if (listen(fd, MaxTCPServerBackLog) == -1) {
            logger.log("listen() failed: %\n", strerror(errno));
            close(fd);
            continue;
          }
        }
      }
      if (socket_cfg.needs_so_timestamp) {
        if (!setSOTimestamp(fd)) {
          logger.log("setSOTimestamp() failed: %\n", strerror(errno));
          close(fd);
          continue;
        }
      }
      freeaddrinfo(res);
      return fd; // success
    }
    freeaddrinfo(res);
    return -1;
  }
}