//
// Created by jemyzhang on 16-11-15.
//

#ifndef SERIAL_OVER_SOCKET_SOCKET_H
#define SERIAL_OVER_SOCKET_SOCKET_H

#include <string>

namespace SerialOverSocket {
class Socket {
public:
  Socket(std::string ipaddr, int port, bool isclient = false);

  ~Socket();

public:
  int setblocking(bool);
  int setunblock() { return setblocking(false); }

  static int setblocking(int, bool);
  static int setunblock(int fd) { return setblocking(fd, false); }

  int fileno() { return fd_; }

private:
  int fd_;
};
} // namespace SerialOverSocket
#endif // SERIAL_OVER_SOCKET_SOCKET_H
