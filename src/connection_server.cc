//
// Created by jemyzhang on 16-11-28.
//

#include "connection_server.h"
#include "ansi_color.h"
#include "ioloop.h"
#include "serialport.h"
#include "snippets.h"
#include "sockserver.h"
#include "version.h"

#include <iostream>
#include <iterator>
#include <sstream>

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
    : Connection(fd, {CONNECTION_ADMIN, host + ":" + port, ""}),
      quit_(false), DataProxy::Client(DataProxy::Client::SOCK2SERIAL),
      processor_thread_(&AdminConnection::cmd_processor, this) {
  write_txbuf("\n");
  write_txbuf(GREEN "===---      Serial Over Socket     ---===\n");
  write_txbuf("===---   Control Panel " UNDERLINE VERSION GREEN "   ---===\n");
  write_txbuf(": Enter \"" YELLOW "help" GREEN "\" for usage hints\n");
  write_txbuf(YELLOW "MGR$ " NONE);
}

AdminConnection::~AdminConnection() {
  std::unique_lock<std::mutex>(this->mutex_), (this->quit_ = true);
  condition_.notify_one();
  processor_thread_.join();
}

void AdminConnection::received(const char *content, ssize_t length) {
  // received from serial port, not process
}

ssize_t AdminConnection::write_rxbuf(const char *content, ssize_t length) {
  std::unique_lock<std::mutex> guard(mutex_);
  bool ignore = false;
  // enable echo
  write_txbuf(content, length);
  // assume input char by char
  if (length == 1) {
    switch (content[0]) {
    case '\n':
      // write_txbuf("MGR$ ");
      break;
    case 0x7f:
      if (rxbuf_.remove_line_char()) {
        write_txbuf(MOVE_LEFT(1) CLRAFTER);
      }
      ignore = true;
      break;
    default:
      break;
    }
  }

  if (ignore)
    return length;

  rxbuf_.append(content, length);
  condition_.notify_one();
  return length;
}

void AdminConnection::cmd_processor() {
  for (;;) {
    std::unique_lock<std::mutex> guard(mutex_);
    condition_.wait(guard, [this]() { return rxbuf_.hasline() || quit_; });
    string cmd_input;
    if (!(cmd_input = rxbuf_.popline()).empty()) {
      std::stringstream ss(cmd_input);
      std::istream_iterator<std::string> begin(ss);
      std::istream_iterator<std::string> end;
      std::vector<std::string> args(begin, end);
      if (args[0] == "help") {
        print_help();
      } else if (args[0] == "disconnect") {
        SerialPort::getInstance()->disconnect();
      } else if (args[0] == "connect") {
        SerialPort::getInstance()->connect();
      } else if (args[0] == "reconnect") {
        SerialPort::getInstance()->reconnect();
      } else if (args[0] == "set") {
        if (args.size() < 3) {
          write_txbuf(
              BOLD_BLUE
              "usage: set baudrate/databits/parity/stopbit [options]\n" NONE);
        } else {
          bool ret = false;
          bool not_supported = false;
          try {
            if (args[1] == "baudrate") {
              ret =
                  SerialPort::getInstance()->set_baudrate(stoi(args[2]), true);
            } else if (args[1] == "databits") {
              ret =
                  SerialPort::getInstance()->set_databits(stoi(args[2]), true);
            } else if (args[1] == "parity") {
              ret = SerialPort::getInstance()->set_parity(stoi(args[2]), true);
            } else if (args[2] == "stopbit") {
              ret = SerialPort::getInstance()->set_stopbit(stoi(args[2]), true);
            } else {
              write_txbuf(BOLD_RED "action not supported\n" NONE);
              not_supported = true;
            }
            if (!not_supported) {
              write_txbuf(ret ? BOLD_GREEN "OK\n" NONE
                              : BOLD_RED "FAIL\n" NONE);
            }
          } catch (std::invalid_argument) {
            write_txbuf(BOLD_RED "FAIL\n" NONE);
          }
        }
      } else if (args[0] == "exit") {
        req_quit_ = true;
        write_txbuf("Bye!\n");
      } else if (args[0] == "ls") {
        string titles = Snippets::getInstance()->ls("  ");
        write_txbuf(titles);
        write_txbuf("\n");
      } else if (args[0] == "cat" && args.size() > 1) {
        string contents = Snippets::getInstance()->cat(args[1], "  ");
        write_txbuf(contents);
        write_txbuf("\n");
      } else {
        if (Snippets::getInstance()->exists(args[0])) {
          string contents = Snippets::getInstance()->cat(args[0]);
          transfer(contents.c_str(), contents.length());
          transfer("\n", 1);
        } else {
          write_txbuf(BOLD_RED "command not found!\n" NONE);
        }
      }
    }
    if (!req_quit_) {
      write_txbuf(YELLOW "MGR$ " NONE);
    }
    if (quit_)
      break;
  }
}

void AdminConnection::print_help() {
  string help_msg =
      "\n"
      "  connect    :  connect to serial port with the existing config\n"
      "  disconnect :  disconnect from serial port\n"
      "  reconnect  :  disconnect and then connect to serial port\n"
      "  ls         :  list snippets\n"
      "  cat <name> :  show contents of snippet\n"
      "  <cmd>      :  run snippet\n"
      "  exit       :  exit the current client connection\n"
      "  help       :  print this help message\n"
      "\n";
  write_txbuf(help_msg);
}
} // namespace SerialOverSocket
