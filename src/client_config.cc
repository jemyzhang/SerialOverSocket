//
// Created by jemyzhang on 17-2-23.
//

#include "client_config.h"
#include <fstream>
#include <iostream>
#include <json.hpp>

using namespace std;
using namespace nlohmann;

namespace SerialOverSocket {

ClientConfig::ServerManager::ServerManager() {
  this->port = 10304;
  this->password = "foobar";
}

ClientConfig::ServerManager::~ServerManager() = default;

ClientConfig::Server::Server() {
  this->address = "0.0.0.0";
  this->port = 10303;
}

ClientConfig::Server::~Server() = default;

bool ClientConfig::load_config_file(const string filename) {
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
