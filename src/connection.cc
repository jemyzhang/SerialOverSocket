//
// Created by jemyzhang on 16-11-28.
//

#include "connection.h"
#include "ioloop.h"
#include <iostream>
#include <unistd.h>

namespace SerialOverSocket {

Connection::Connection(int fd, AdditionalInfo info)
    : fd_(fd), info_(info), req_quit_(false) {

  if (info_.type == CONNECTION_SERIAL) {
    cout << "[SerialPort] connected to " << info_.dev << endl;
  } else {
    if (info_.dev.length() > 0) {
      cout << "[";
      cout << (info_.type == CONNECTION_CLIENT ? "Client" : "Admin");
      cout << "] connected from " << info_.dev << endl;
    }
  }
}

Connection::~Connection() {
  if (fd_ > 0) {
    close(fd_);
    if (info_.dev.length() > 0) {
      if (info_.type == CONNECTION_SERIAL) {
        cout << "[SerialPort] ";
      } else {
        cout << "[";
        cout << (info_.type == CONNECTION_CLIENT ? "Client" : "Admin");
        cout << "] ";
      }
      cout << info_.dev << " disconnected" << endl;
    }
  }
  rxbuf_.reset();
  txbuf_.reset();
}

ssize_t Connection::write_txbuf(const char *content, ssize_t length) {
  ssize_t len = txbuf_.append(content, length);

  // trigger output event
  IOLoop::getInstance().get()->modifyHandler(fd_, EPOLLOUT | EPOLLIN | EPOLLET);
  return len;
}

bool Connection::quiting() { return req_quit_; }

void Connection::flush() {
  char c;
  while (!txbuf_.empty()) {
    ssize_t written = write(fd_, txbuf_.contents(), txbuf_.length());
    if(written <= 0) break;
    txbuf_.pop(written);
  }
}

ssize_t Connection::write_txbuf(const string &s) {
  return write_txbuf(s.c_str(), s.length());
}
}
