//
// Created by jemyzhang on 16-12-30.
//

#ifndef SERIAL_OVER_SOCKET_SOCKET_CLIENT_H
#define SERIAL_OVER_SOCKET_SOCKET_CLIENT_H

#include "config.h"
#include "connection.h"
#include "handler.h"
#include "socket.h"
#include <termio.h>

using namespace std;

namespace SerialOverSocket {

class Client : public Handler, public Connection {
public:
  explicit Client();

  ~Client() final;

  int handle(epoll_event e) final;

  ssize_t write_rxbuf(const char *content, ssize_t length) final;

private:
  int input_data_handler(int fd);

  int input_event_handler(int fd);

  int output_data_handler();

  void set_termio_mode();

private:
  void switch_admin_connection(bool connect);

  void process_console_input(const char *content, ssize_t length);

  void handle_cmd(string cmd);

  static void print_help();

private:
  shared_ptr<ClientConfig> cfg;
  Socket client_socket_;
  unique_ptr<Socket> admin_socket_;
  struct termios term_option_;
  int client_fd_;    // for backup
  string client_buf; // store input content from serial port while switched to
  // admin socket
private:
  vector<string> history_;
  int history_idx_;
  string cmdline_;
  int cursor_pos_;

private:
  void start_log(string path);

  void stop_log();

  int log_fd_;
  string log_file_;
};
} // namespace SerialOverSocket

#endif // SERIAL_OVER_SOCKET_SOCKET_CLIENT_H
