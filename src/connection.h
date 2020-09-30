//
// Created by jemyzhang on 16-11-28.
//

#ifndef SERIAL_OVER_SOCKET_CONNECTION_H
#define SERIAL_OVER_SOCKET_CONNECTION_H

#include "charbuf.h"

using namespace std;

namespace SerialOverSocket {

  class ConnectionManager;

  class Connection {
  public:
    enum ConnectionType {
      CONNECTION_CLIENT,
      CONNECTION_ADMIN,
      CONNECTION_SERIAL,
    };

    struct AdditionalInfo {
      ConnectionType type;
      string dev;
      string extra;
    };

    Connection(int fd, AdditionalInfo info);

    virtual ~Connection();

    ConnectionType type() { return info_.type; }

  public:
    virtual ssize_t write_txbuf(const string &content);

    virtual ssize_t write_txbuf(const char *content, ssize_t length);

    virtual ssize_t write_rxbuf(const char *content, ssize_t length) = 0;

    void flush();

    bool quiting();

  protected:
    int fd_;
    CharBuf txbuf_;
    CharBuf rxbuf_;

    bool req_quit_; /* request quit */
    AdditionalInfo info_;

    friend class ConnectionManager;
  };
}

#endif // SERIAL_OVER_SOCKET_CONNECTION_H
