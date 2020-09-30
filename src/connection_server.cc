//
// Created by jemyzhang on 16-11-28.
//

#include <iostream>
#include <iterator>
#include <sstream>
#include <sys/types.h>

#include "ansi_color.h"
#include "config.h"
#include "connection_server.h"
#include "serialport.h"
#include "version.h"

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

#define MGR_PROMPT_STR "MGR$ "
#define MGR_PROMPT YELLOW MGR_PROMPT_STR NONE

  AdminConnection::AdminConnection(int fd, string host, string port)
     : Connection(fd, {CONNECTION_ADMIN, host + ":" + port, ""}),
       quit_(false), DataProxy::Client(DataProxy::Client::SOCK2SERIAL) {
    write_txbuf("\n");
    write_txbuf(GREEN "===---      Serial Over Socket     ---===\n");
    write_txbuf("===---   Control Panel " UNDERLINE VERSION GREEN "   ---===\n");
    write_txbuf(": Enter \"" YELLOW "help" GREEN "\" for usage hints\n");
    write_txbuf(MGR_PROMPT);
  }

  AdminConnection::~AdminConnection() {
  }

  void AdminConnection::received(const char *content, ssize_t length) {
    // received from serial port, not process
  }

  ssize_t AdminConnection::write_rxbuf(const char *content, ssize_t length) {
    string cmd_input(content, length);
    if (length == 1 && content[0] == '\n') {
      write_txbuf(MGR_PROMPT);
    } else {
      process_cmd(cmd_input);
      write_txbuf(MGR_PROMPT);
    }
    return length;
  }

  void AdminConnection::process_cmd(string cmd_input) {
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
            int v = stoi(args[2]);
            ret =
               SerialPort::getInstance()->set_baudrate(v, true);
            if (ret) {
              ServerConfig::getInstance()->serial_baudrate(v);
            }
          } else if (args[1] == "databits") {
            int v = stoi(args[2]);
            ret =
               SerialPort::getInstance()->set_databits(v, true);
            if (ret) {
              ServerConfig::getInstance()->serial_databits(v);
            }
          } else if (args[1] == "parity") {
            char v = stoi(args[2]);
            ret = SerialPort::getInstance()->set_parity(v, true);
            if (ret) {
              ServerConfig::getInstance()->serial_parity(v);
            }
          } else if (args[2] == "stopbit") {
            int v = stoi(args[2]);
            ret = SerialPort::getInstance()->set_stopbit(v, true);
            if (ret) {
              ServerConfig::getInstance()->serial_stopbit(v);
            }
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
    } else if (args[0] == "save") {
      ServerConfig::getInstance()->save();
      write_txbuf("Saved!\n");
    } else if (args[0] == "ts") {
      bool t = ServerConfig::getInstance()->timestamp();
      ServerConfig::getInstance()->timestamp(!t);
      t = ServerConfig::getInstance()->timestamp();
      write_txbuf("Show timestamp: ");
      write_txbuf(t ? BOLD_GREEN "ON" NONE :
                  BOLD_RED "OFF" NONE);
      write_txbuf("\n");
    } else if (args[0] == "exit") {
      req_quit_ = true;
      write_txbuf("Bye!\n");
    } else {
      write_txbuf(BOLD_RED);
      write_txbuf(args[0]);
      write_txbuf(": ");
      write_txbuf("command not found!\n" NONE);
    }
  }

  void AdminConnection::print_help() {
    string help_msg =
       "[remote commands]\n"
       "  connect    :  connect to serial port with the existing config\n"
       "  disconnect :  disconnect from serial port\n"
       "  reconnect  :  disconnect and then connect to serial port\n"
       "  set        :  set baudrate/databits/parity/stopbit [options]\n"
       "  save       :  save configuration\n"
       "  ts         :  enable/disable timestamp"
       "\n";
    write_txbuf(help_msg);
  }
} // namespace SerialOverSocket
