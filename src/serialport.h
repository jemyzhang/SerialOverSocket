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
#include <termios.h>
#include <vector>

using namespace std;

namespace SerialOverSocket {
  class SerialPort {
  public:
    SerialPort();

    ~SerialPort();

    using OnStatusChangedCallback = std::function<void(bool, int)>;

  public:
    int connect(shared_ptr<ServerConfig> cfg);

    int connect() { return connect(cfg_); }

    void disconnect();

    int reconnect();

    bool isconnected() { return fd_ > 0; }

    int fileno() { return fd_; }

    bool set_baudrate(int baudrate, bool instantly = false);

    bool set_databits(int databits, bool instantly = false);

    bool set_parity(char parity, bool instantly = false);

    bool set_stopbit(char stopbit, bool instantly = false);

    void InstallStatusCallback(const OnStatusChangedCallback &callback) {
      callback_.insert(callback_.begin(), callback);
    }

  public:
    static shared_ptr<SerialPort> getInstance();

  private:
    int fd_;
    shared_ptr<ServerConfig> cfg_;
    vector<OnStatusChangedCallback> callback_;
    static shared_ptr<SerialPort> instance_;
    struct termios options_;
  };
} // namespace SerialOverSocket

#endif // SERIAL_OVER_SOCKET_SERIALPORT_H
