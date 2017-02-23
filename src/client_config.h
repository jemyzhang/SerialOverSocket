//
// Created by jemyzhang on 17-2-23.
//

#ifndef SERIAL_OVER_SOCKET_CLIENT_CONFIG_H
#define SERIAL_OVER_SOCKET_CLIENT_CONFIG_H

#include <string>
using namespace std;

namespace SerialOverSocket {

class ClientConfig {
public:
  ClientConfig() = default;
  ~ClientConfig() = default;

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

  struct Info {
    struct Server server;
  };

public:
  bool load_config_file(const string filepath);

public:
  struct Info detail_;
};

} // namespace SerialOverSocket

#endif // SERIAL_OVER_SOCKET_CLIENT_CONFIG_H
