//
// Created by jemyzhang on 16-11-15.
//

#include "socket.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/tcp.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <unistd.h>

namespace SerialOverSocket {
  Socket::Socket(const std::string ipaddr, int port, bool isclient) {
    fd_ = -1;
    if (port > 0) {
      struct sockaddr_in ip_addr{};
      int opt_val = 1;
      socklen_t opt_len = sizeof(opt_val);

      // create socket
      fd_ = socket(AF_INET, SOCK_STREAM, 0);
      if (fd_ < 0) {
        perror("create socket");
        return;
      }

      // set socket options
      if (setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &opt_val, opt_len) < 0) {
        perror("socket opt, TCP_NODELAY");
        goto errorclose;
      }

      if (setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt_val, opt_len) < 0) {
        perror("socket opt, SO_KEEPALIVE");
        goto errorclose;
      }

      if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt_val, opt_len) < 0) {
        perror("socket opt, SO_REUSEADDR");
        goto errorclose;
      }

      memset(&ip_addr, 0, sizeof(ip_addr));
      ip_addr.sin_family = AF_INET;
      if (inet_aton(ipaddr.c_str(), &ip_addr.sin_addr) == 0) {
        perror("ip address");
        goto errorclose;
      }
      ip_addr.sin_port = htons(static_cast<uint16_t>(port));
      if (!isclient) { // server
        if (ioctl(fd_, FIONBIO, &opt_val) < 0) {
          // set non-block mode
          perror("set nonblock");
          goto errorclose;
        }
        // bind address
        if (bind(fd_, (struct sockaddr *) &ip_addr, sizeof(ip_addr)) < 0) {
          perror("bind");
          goto errorclose;
        }

        if (listen(fd_, SOMAXCONN) < 0) {
          perror("listen");
          goto errorclose;
        }
      } else { // client
        if (connect(fd_, (struct sockaddr *) &ip_addr, sizeof(ip_addr)) < 0) {
          perror("connect to server");
          goto errorclose;
        }
      }
      return;

      errorclose:
      shutdown(fd_, SHUT_RDWR);
      close(fd_);
      fd_ = -1;
      return;
    } else {
      fd_ = socket(AF_UNIX, SOCK_STREAM, 0);

      struct sockaddr_un sock_addr{};
      memset(&sock_addr, 0, sizeof(sock_addr));
      sock_addr.sun_family = AF_UNIX;
      strncpy(sock_addr.sun_path, ipaddr.c_str(), sizeof(sock_addr.sun_path) - 1);

      if (!isclient) {
        if (bind(fd_, (struct sockaddr *) &sock_addr,
                 sizeof(struct sockaddr_un))) {
          perror("binding name to datagram socket");
          exit(1);
        }
        if (listen(fd_, SOMAXCONN) < 0) {
          perror("listen");
          exit(1);
        }
      } else {
        if (connect(fd_, (struct sockaddr *) &sock_addr, sizeof(sock_addr))) {
          perror("connecting name to datagram socket");
          close(fd_);
          fd_ = -1;
          return;
        }
      }
      return;
    }
  }

  Socket::~Socket() {
    if (fd_) {
      shutdown(fd_, SHUT_RDWR);
    }
  }

  int Socket::setblocking(bool b_block) {
    return Socket::setblocking(this->fd_, b_block);
  }

  int Socket::setblocking(int fd, bool b_block) {
    int flags, s;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
      perror("fcntl");
      return -1;
    }

    if (b_block) {
      flags &= ~O_NONBLOCK;
    } else {
      flags |= O_NONBLOCK;
    }
    s = fcntl(fd, F_SETFL, flags);
    if (s == -1) {
      perror("fcntl");
      return -1;
    }

    return 0;
  }
} // namespace SerialOverSocket
