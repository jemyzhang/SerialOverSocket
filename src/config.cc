//
// Created by jemyzhang on 17-2-23.
//

#include "config.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <pwd.h>

using namespace std;

namespace SerialOverSocket {
  shared_ptr<Config> Config::instance_ = nullptr;

  shared_ptr<Config> Config::getInstance() {
    if (!instance_) {
      instance_ = make_shared<Config>();
    }
    return instance_;
  }

  bool Config::load_config_file(string filename) {
    try {
      if (filename.empty()) {
        struct passwd *pw = getpwuid(getuid());
        filename = string(pw->pw_dir) + "/.config/sos/config.json";
      }
      cfgfile = filename;
      ifstream stream_json(filename);
      if (!stream_json.is_open()) {
        throw ("config file not found");
      }
      stream_json >> this->configs;
    } catch (detail::parse_error &e) {
      cerr << e.what() << endl;
      exit(1);
    } catch (detail::type_error &e) {
      cerr << e.what() << endl;
      exit(1);
    } catch (detail::out_of_range &e) {
      cerr << "error happend: " << e.what() << endl;
      exit(1);
    } catch (const char *str) {
      cerr << str << endl;
      exit(1);
    }
    return true;
  }

  string Config::server_address() {
    json node = configs.at(json::json_pointer("/server/address"));
    string value = "127.0.0.1";
    if(!node.empty()) {
      value = node.get<string>();
    }
    return value;
  }

  int Config::server_port() {
    json node = configs.at(json::json_pointer("/server/port"));
    int value = 10303;
    if(!node.empty()) {
      value = node.get<int>();
    }
    return value;
  }

  string Config::admin_password() {
    json node = configs.at(json::json_pointer("/admin/password"));
    string value = "foobar";
    if(!node.empty()) {
      value = node.get<string>();
    }
    return value;
  }

  int Config::admin_port() {
    json node = configs.at(json::json_pointer("/admin/port"));
    int value = 10304;
    if(!node.empty()) {
      value = node.get<int>();
    }
    return value;
  }

  shared_ptr<ServerConfig> ServerConfig::getInstance() {
    shared_ptr<Config> base = Config::getInstance();
    return static_pointer_cast<ServerConfig>(base);
  }

  bool ServerConfig::timestamp() {
    bool value = false;
    try {
        json node = configs.at(json::json_pointer("/serialport/timestamp"));
        if(!node.empty()) {
            value = node.get<bool>();
        }
    } catch (...) {
        timestamp(value);
    }
    return value;

  }

  string ServerConfig::serial_device() {
    json node = configs.at(json::json_pointer("/serialport/device"));
    string value = "/dev/ttyUSB0";
    if(!node.empty()) {
      value = node.get<string>();
    }
    return value;

  }
  int ServerConfig::serial_baudrate() {
    json node = configs.at(json::json_pointer("/serialport/baudrate"));
    int value = 115200;
    if(!node.empty()) {
      value = node.get<int>();
    }
    return value;
  }
  int ServerConfig::serial_databits() {
    json node = configs.at(json::json_pointer("/serialport/databits"));
    int value = 8;
    if(!node.empty()) {
      value = node.get<int>();
    }
    return value;
  }
  int ServerConfig::serial_stopbit() {
    json node = configs.at(json::json_pointer("/serialport/stopbit"));
    int value = 1;
    if(!node.empty()) {
      value = node.get<int>();
    }
    return value;
  }

  char ServerConfig::serial_parity() {
    json node = configs.at(json::json_pointer("/serialport/parity"));
    string value = "n";
    if(!node.empty()) {
      value = node.get<string>();
    }
    return value.at(0);
  }

  void ServerConfig::timestamp(bool t){
      json backup = configs;
      try {
          json &j = configs[json::json_pointer("/serialport/timestamp")];
          j = t;
      } catch (...) {
          configs = backup;
      }
  }

  void ServerConfig::serial_device(string dev){
      json backup = configs;
      try {
          json &j = configs[json::json_pointer("/serialport/device")];
          j = dev;
      } catch (...) {
          configs = backup;
      }
  }

  void ServerConfig::serial_baudrate(int b){
      json backup = configs;
      try {
          json &j = configs[json::json_pointer("/serialport/baudrate")];
          j = b;
      } catch (...) {
          configs = backup;
      }
  }

  void ServerConfig::serial_databits(int d){
      json backup = configs;
      try {
          json &j = configs[json::json_pointer("/serialport/databits")];
          j = d;
      } catch (...) {
          configs = backup;
      }
  }

  void ServerConfig::serial_stopbit(int s){
      json backup = configs;
      try {
          json &j = configs[json::json_pointer("/serialport/stopbit")];
          j = s;
      } catch (...) {
          configs = backup;
      }
  }

  void ServerConfig::serial_parity(char p){
      json backup = configs;
      try {
          json &j = configs[json::json_pointer("/serialport/parity")];
          string s(1,p);
          j = s;
      } catch (...) {
          configs = backup;
      }
  }

  void ServerConfig::save(){
      try {
          ofstream stream_json(cfgfile);
          if(!stream_json.is_open()) {
              throw("failed to open config file");
          }
          stream_json << setw(4) << configs << endl;
      } catch (const char *str) {
          cerr << str << endl;
      }

  }

  shared_ptr<ClientConfig> ClientConfig::getInstance() {
    shared_ptr<Config> base = Config::getInstance();
    return static_pointer_cast<ClientConfig>(base);
  }

} // namespace SerialOverSocket
