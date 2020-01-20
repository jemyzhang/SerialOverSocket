//
// Created by jemyzhang on 17-2-23.
//

#ifndef SERIAL_OVER_SOCKET_CONFIG_H
#define SERIAL_OVER_SOCKET_CONFIG_H

#include <string>
#include <memory>
#include <json.hpp>
using namespace std;
using namespace nlohmann;

namespace SerialOverSocket {

  class Config {
  public:
    Config() = default;
    ~Config() = default;

  public:
    static shared_ptr<Config> getInstance();

  public:
    string server_address();
    int server_port();
    int admin_port();
    string admin_password();

  public:
    bool load_config_file(string filepath);

  public:
    static shared_ptr<Config> instance_;
  protected:
    json configs;
  };

  class ServerConfig : public Config {
  public:
    ServerConfig() = default;
    ~ServerConfig() = default;

  public:
    static shared_ptr<ServerConfig> getInstance();

  public:
    string serial_device();
    int serial_baudrate();
    int serial_databits();
    int serial_stopbit();
    char serial_parity();
  };

  class ClientConfig : public Config {
  public:
    static shared_ptr<ClientConfig> getInstance();

  public:
    ClientConfig() = default;
    ~ClientConfig() = default;
  };
} // namespace SerialOverSocket

#endif // SERIAL_OVER_SOCKET_CONFIG_H
