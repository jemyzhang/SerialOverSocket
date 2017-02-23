//
// Created by jemyzhang on 17-6-13.
//

#ifndef SERIAL_OVER_SOCKET_HANDLER_H
#define SERIAL_OVER_SOCKET_HANDLER_H

#include <sys/epoll.h>

namespace SerialOverSocket {
class Handler {
public:
  Handler();

  virtual ~Handler();

  virtual int handle(epoll_event e) = 0;
};
}

#endif // SERIAL_OVER_SOCKET_HANDLER_H
