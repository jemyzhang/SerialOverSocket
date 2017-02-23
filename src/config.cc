//
// Created by jemyzhang on 17-2-23.
//

#include "config.h"
#include <fstream>
#include <iostream>
#include <json.hpp>

using namespace std;
using namespace nlohmann;

namespace SerialOverSocket {

Config::ServerManager::ServerManager() {
  this->port = 10304;
  this->password = "foobar";
}

Config::ServerManager::~ServerManager() = default;

Config::SerialPort::SerialPort() {
  this->devname = "/dev/ttyUSB0";
  this->baudrate = 115200;
  this->databits = 8;
  this->parity = 'n';
  this->stopbit = 1;
}

Config::SerialPort::~SerialPort() = default;

Config::Server::Server() {
  this->address = "0.0.0.0";
  this->port = 10303;
}

Config::Server::~Server() = default;

bool Config::load_config_file(string filename) {
  try {
    ifstream stream_json(filename);
    if (!stream_json.is_open()) {
      throw("config file not found");
    }
    json js;
    stream_json >> js;

    detail_.server.manager.port = js["server"]["manager"]["port"];
    detail_.server.manager.password = js["server"]["manager"]["password"];

    detail_.server.address = js["server"]["address"];
    detail_.server.port = js["server"]["port"];

    detail_.serial.devname = js["serialport"]["device"];
    detail_.serial.baudrate = js["serialport"]["baudrate"];
    detail_.serial.databits = js["serialport"]["databits"];
    detail_.serial.stopbit = js["serialport"]["stopbit"];
    string parity_string = js["serialport"]["parity"];
    detail_.serial.parity = parity_string.at(0);
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

} // namespace SerialOverSocket
