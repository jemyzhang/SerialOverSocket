//
// Created by jemyzhang on 16-12-30.
//

#ifndef SERIAL_OVER_SOCKET_SOCKET_CLIENT_H
#define SERIAL_OVER_SOCKET_SOCKET_CLIENT_H

#include "client_config.h"
#include "connection.h"
#include "handler.h"
#include "socket.h"
#include <termio.h>

using namespace std;

namespace SerialOverSocket {

class Client : public Handler, public Connection {
public:
  explicit Client(ClientConfig::Info &info);
  ~Client() final;

  int handle(epoll_event e) final;

  ssize_t write_rxbuf(const char *content, ssize_t length) final;

private:
  int input_data_handler(int fd);
  int input_event_handler(int fd);
  int output_data_handler();

  void set_termio_mode();

private:
  Socket client_socket_;
  struct termios term_option_;
};
}

#endif // SERIAL_OVER_SOCKET_SOCKET_CLIENT_H
