//
// Created by jemyzhang on 17-2-23.
//

#ifndef SERIAL_OVER_SOCKET_CONFIG_H
#define SERIAL_OVER_SOCKET_CONFIG_H

#include <string>
using namespace std;

namespace SerialOverSocket {

class Config {
public:
  Config() = default;
  ~Config() = default;

public:
  struct ServerManager {
    ServerManager();
    ~ServerManager();

    int port;
    string password;
  };

  struct Server {
    Server();
    ~Server();

    string address;
    int port;
    struct ServerManager manager;
  };

  struct SerialPort {
    SerialPort();
    ~SerialPort();

    string devname;
    int baudrate;
    int databits;
    int stopbit;
    int parity;
  };

  struct Info {
    struct Server server;
    struct SerialPort serial;
  };

public:
  bool load_config_file(string filepath);

public:
  struct Info detail_;
};

} // namespace SerialOverSocket

#endif // SERIAL_OVER_SOCKET_CONFIG_H
