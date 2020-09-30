//
// Created by jemyzhang on 16-11-28.
//

#ifndef SERIAL_OVER_SOCKET_CONNECTION_SERVER_H
#define SERIAL_OVER_SOCKET_CONNECTION_SERVER_H

#include "charbuf.h"
#include "connection.h"
#include "dataproxy.h"
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std;

namespace SerialOverSocket {

  class ClientConnection : public Connection, public DataProxy::Client {
  public:
    explicit ClientConnection(int fd, string host, string port);

    ~ClientConnection() final;

  public:
    ssize_t write_rxbuf(const char *content, ssize_t length);

    // implementation of DataProxy::Client
    void received(const char *content, ssize_t length);
  };

  class SerialPortConnection : public Connection, public DataProxy::Client {
  public:
    explicit SerialPortConnection(int fd, string dev, string detail);

    ~SerialPortConnection() final;

  public:
    ssize_t write_rxbuf(const char *content, ssize_t length) final;

    // implementation of DataProxy::Client
    void received(const char *content, ssize_t length) final;
  };

  class AdminConnection : public Connection, public DataProxy::Client {
  public:
    explicit AdminConnection(int fd, string host, string port);

    ~AdminConnection() final;

  public:
    ssize_t write_rxbuf(const char *content, ssize_t length) final;

    // implementation of DataProxy::Client
    void received(const char *content, ssize_t length) final;

  private:
    void process_cmd(string cmd);

    void print_help();

    bool quit_;
  };

  class ConnectionManager {
  public:
    ConnectionManager();

    ~ConnectionManager();

  public:
    int append(Connection *pinfo);

    int remove(Connection *pinfo);

    Connection *remove(int fd);

    Connection *pop();

    Connection *find(int fd);

  private:
    vector<Connection *> connections_;
    mutex mutex_;
  };
} // namespace SerialOverSocket

#endif // SERIAL_OVER_SOCKET_CONNECTION_SERVER_H
