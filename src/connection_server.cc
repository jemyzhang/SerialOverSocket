//
// Created by jemyzhang on 16-11-28.
//

#include "connection_server.h"
#include "ioloop.h"
#include "serialport.h"
#include "sockserver.h"
#include <iostream>

using namespace std::placeholders;

namespace SerialOverSocket {

ConnectionManager::ConnectionManager() { connections_.clear(); }

ConnectionManager::~ConnectionManager() {
  mutex_.lock();
  // TODO: need to free space here?
  connections_.clear();
  mutex_.unlock();
}

int ConnectionManager::append(Connection *pinfo) {
  mutex_.lock();
  connections_.insert(connections_.end(), pinfo);
  mutex_.unlock();
  return 0;
}

Connection *ConnectionManager::pop() {
  Connection *p = nullptr;
  mutex_.lock();
  p = connections_.front();
  connections_.erase(connections_.begin());
  mutex_.unlock();
  return p;
}

Connection *ConnectionManager::remove(int fd) {
  Connection *pret = nullptr;
  mutex_.lock();
  for (auto i = connections_.begin(); i != connections_.end(); i++) {
    Connection *p = *i;
    if (p->fd_ == fd) {
      connections_.erase(i);
      pret = p;
      break;
    }
  }
  mutex_.unlock();
  return pret;
}

int ConnectionManager::remove(Connection *pinfo) {
  int fd = 0;
  if (pinfo) {
    fd = pinfo->fd_;
    remove(fd);
  }
  return fd;
}

Connection *ConnectionManager::find(int fd) {
  Connection *pret = nullptr;
  mutex_.lock();
  for (auto p : connections_) {
    if (p->fd_ == fd) {
      pret = p;
      break;
    }
  }
  mutex_.unlock();
  return pret;
}

ClientConnection::ClientConnection(int fd, string host, string port)
    : Connection(fd, {CONNECTION_CLIENT, host + ":" + port, ""}),
      DataProxy::Client(DataProxy::Client::SOCK2SERIAL) {}

ClientConnection::~ClientConnection() = default;

ssize_t ClientConnection::write_rxbuf(const char *content, ssize_t length) {
  transfer(content, length);
  return length;
}

void ClientConnection::received(const char *content, ssize_t length) {
  write_txbuf(content, length);
}

SerialPortConnection::SerialPortConnection(int fd, string dev, string detail)
    : Connection(fd, {CONNECTION_SERIAL, dev, detail}),
      DataProxy::Client(DataProxy::Client::SERIAL2SOCK) {}

SerialPortConnection::~SerialPortConnection() = default;

ssize_t SerialPortConnection::write_rxbuf(const char *content, ssize_t length) {
  transfer(content, length);
  return length;
}

void SerialPortConnection::received(const char *content, ssize_t length) {
  write_txbuf(content, length);
}

AdminConnection::AdminConnection(int fd, string host, string port)
    : Connection(fd, {CONNECTION_ADMIN, host + ":" + port, ""}), quit_(false),
      processor_thread_(&AdminConnection::cmd_processor, this) {
  write_txbuf("===---      Serial Over Socket     ---===\n");
  write_txbuf("===--- Admin Interface Ver." SERVER_VER " --===\n");
  write_txbuf("Enter \"help\" for usage hints\n");
}

AdminConnection::~AdminConnection() {
  std::unique_lock<std::mutex>(this->mutex_), (this->quit_ = true);
  condition_.notify_one();
  processor_thread_.join();
}

ssize_t AdminConnection::write_rxbuf(const char *content, ssize_t length) {
  std::unique_lock<std::mutex> guard(mutex_);
  rxbuf_.append(content, length);
  condition_.notify_one();
  return length;
}

void AdminConnection::cmd_processor() {
  for (;;) {
    std::unique_lock<std::mutex> guard(mutex_);
    condition_.wait(guard, [this]() { return !rxbuf_.empty() || quit_; });
    if (!rxbuf_.empty()) {
      string cmd_input;
      while (!(cmd_input = rxbuf_.popline()).empty()) {
        if (cmd_input == "help") {
          // print_help();
        } else if (cmd_input == "disconnect") {
          SerialPort::getInstance()->disconnect();
        } else if (cmd_input == "connect") {
          SerialPort::getInstance()->connect();
        } else if (cmd_input == "reconnect") {
          SerialPort::getInstance()->reconnect();
        } else if (cmd_input == "config") {
        }
      }
    }
    if (quit_)
      break;
  }
}
}
