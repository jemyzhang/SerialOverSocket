//
// Created by jemyzhang on 16-12-30.
//

#ifndef SERIAL_OVER_SOCKET_SOCKSERVER_H
#define SERIAL_OVER_SOCKET_SOCKSERVER_H

#include "config.h"
#include "connection_server.h"
#include "handler.h"
#include "socket.h"
#include <vector>

#define SERVER_VER "1.0.0"

using namespace std;

namespace SerialOverSocket {
class Server : Handler {
public:
  explicit Server(Config::Info &info);

  ~Server() final;

  int handle(epoll_event e) final;

private:
  int close_connection(int fd);
  int close_connection(Connection *);
  void close_all_connections(bool bexit);
  int accept_connection(int srvfd);
  int input_data_handler(int fd);
  int input_event_handler(int fd);
  int output_data_handler(int fd);

  void OnSerialPortConnectionChanged(bool, int);

private:
  Socket server_socket_;
  Socket manager_socket_;
  ConnectionManager *pcmgr_;
};
}

#endif // SERIAL_OVER_SOCKET_SOCKSERVER_H
