//
// Created by jemyzhang on 17-6-13.
//

#ifndef SERIAL_OVER_SOCKET_SERIALPORT_H
#define SERIAL_OVER_SOCKET_SERIALPORT_H
#include "config.h"
#include "handler.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

using namespace std;

namespace SerialOverSocket {
class SerialPort {
public:
  SerialPort();
  ~SerialPort();

  using OnStatusChangedCallback = std::function<void(bool, int)>;

public:
  int connect(Config::SerialPort &);
  int connect() { return connect(cfg_); }
  void setConfig(Config::SerialPort &cfg) { cfg_ = cfg; }
  const Config::SerialPort getConfig() { return cfg_; }

  void disconnect();
  int reconnect();

  bool isconnected() { return fd_ > 0; }

  int fileno() { return fd_; }

  void InstallStatusCallback(const OnStatusChangedCallback &callback) {
    callback_.insert(callback_.begin(), callback);
  }

public:
  static shared_ptr<SerialPort> getInstance();

private:
  int fd_;
  Config::SerialPort cfg_;
  vector<OnStatusChangedCallback> callback_;
  static shared_ptr<SerialPort> instance_;
};
}

#endif // SERIAL_OVER_SOCKET_SERIALPORT_H
