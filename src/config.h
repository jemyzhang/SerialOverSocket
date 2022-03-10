//
// Created by jemyzhang on 17-2-23.
//

#ifndef SERIAL_OVER_SOCKET_CONFIG_H
#define SERIAL_OVER_SOCKET_CONFIG_H

#include <json.hpp>
#include <memory>
#include <string>

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
  string cfgfile;
};

class ServerConfig : public Config {
public:
  ServerConfig() = default;

  ~ServerConfig() = default;

public:
  static shared_ptr<ServerConfig> getInstance();

public:
  // get
  string serial_device();

  int serial_baudrate();

  int serial_databits();

  int serial_stopbit();

  char serial_parity();

  bool timestamp();

  // set
  void serial_device(string);

  void serial_baudrate(int);

  void serial_databits(int);

  void serial_stopbit(int);

  void serial_parity(char);

  void timestamp(bool);

  // save
  void save();
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
